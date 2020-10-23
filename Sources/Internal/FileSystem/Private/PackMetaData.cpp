#include "Engine/Engine.h"
#include "FileSystem/Private/PackMetaData.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/DynamicMemoryFile.h"
#include "FileSystem/FileSystem.h"
#include "Compression/LZ4Compressor.h"
#include "Base/Exception.h"
#include "Debug/DVAssert.h"
#include "Utils/Utils.h"
#include "Utils/StringFormat.h"
#include "Logger/Logger.h"

#include <sqlite_modern_cpp.h>

namespace DAVA
{
Vector<String> FileNamesTree::GetPathElements(const String& relativeFilePath)
{
    // TODO better in future use string_view
    Vector<String> tokens;
    const size_t countNames = 1 + std::count_if(begin(relativeFilePath), end(relativeFilePath), [](char value) { return value == '/'; });
    tokens.reserve(countNames);
    Split(relativeFilePath, "/", tokens);
    return tokens;
}

void FileNamesTree::Add(const String& relativeFilePath)
{
    Vector<String> tokens = GetPathElements(relativeFilePath);
    Node* currentNode = &treeRoot;
    for (String& element : tokens)
    {
        Node& node = currentNode->children[element];
        currentNode = &node;
    }
}

bool FileNamesTree::Find(const String& relativeFilePath) const
{
    Vector<String> tokens = GetPathElements(relativeFilePath);
    const Node* currentNode = &treeRoot;
    for (String& element : tokens)
    {
        const auto it = currentNode->children.find(element);
        if (it != end(currentNode->children))
        {
            currentNode = &it->second;
        }
        else
        {
            return false;
        }
    }
    return true;
}

PackMetaData::PackMetaData(const void* ptr, std::size_t size, const String& fileNames)
{
    Deserialize(ptr, size);

    const size_t sizeOfNames = fileNames.size();
    size_t index = 0;

    while (index < fileNames.size())
    {
        // TODO in future do it with std::string_view
        const char* filePath = &fileNames[index];
        namesTree.Add(filePath);
        // start of next substring after null character
        index = fileNames.find('\0', index + 1) + 1;
    }
}

Vector<uint32> PackMetaData::ConvertStringWithNumbersToVector(const String& dependencies) const
{
    Vector<uint32> result;

    const String delimiter(", ");

    Vector<String> requestNamesStr;
    Split(dependencies, delimiter, requestNamesStr);

    // convert every name from string representation of index to packName
    for (String& pack : requestNamesStr)
    {
        unsigned long i = 0;
        try
        {
            i = stoul(pack);
        }
        catch (std::exception& ex)
        {
            const String errStr = Format("bad dependency index: value: %s, error: %s.", pack.c_str(), ex.what());
            Logger::Error("%s", errStr.c_str());
            DAVA_THROW(Exception, errStr);
        }

        uint32 index = static_cast<uint32>(i);
        result.push_back(index);
    }

    SortAndEraseDuplicates(result);
    return result;
}

PackMetaData::PackMetaData(const FilePath& metaDb)
{
    // if metaDB path inside Android assets folder -> copy it to real file system
    // sqlite can't read files from APK
    FileSystem* fs = GetEngineContext()->fileSystem;
    String pathToMetaDb;
    if (fs->ExistsInAndroidAssets(metaDb))
    {
        FilePath userDoc = fs->GetUserDocumentsPath() + "tmp_extracted_pack_local_meta.db";
        if (fs->CopyFile(metaDb, userDoc, true))
        {
            pathToMetaDb = userDoc.GetAbsolutePathname();
        }
        else
        {
            DAVA_THROW(Exception, "can't extract sqlite db from android assets: " + metaDb.GetStringValue());
        }
    }
    else
    {
        if (fs->IsFile(metaDb))
        {
            pathToMetaDb = metaDb.GetAbsolutePathname();
        }
        else
        {
            DAVA_THROW(Exception, "No local meta file: %s" + pathToMetaDb);
        }
    }
    // extract tables from sqlite DB
    sqlite::database db(pathToMetaDb);

    size_t numIndexes = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Note! Use long long instead of DAVA::int64 as on linux 64bit DAVA::int64 is long.    //
    // But function sqlite::get_col_from_db has no specialization/overload for `long` type. //
    //////////////////////////////////////////////////////////////////////////////////////////

    db << "SELECT count(*) FROM files"
    >> [&](long long countIndexes)
    {
        DVASSERT(countIndexes > 0);
        numIndexes = static_cast<size_t>(countIndexes);
    };

    packIndexes.reserve(numIndexes);

    db << "SELECT path, pack_index FROM files"
    >> [&](std::string path, int packIndex)
    {
        namesTree.Add(path);
        packIndexes.push_back(packIndex);
    };

    size_t numPacks = 0;

    db << "SELECT count(*) FROM packs"
    >> [&](long long countPacks)
    {
        DVASSERT(countPacks > 0);
        numPacks = static_cast<size_t>(countPacks);
    };

    packDependencies.reserve(numPacks);

    db << "SELECT name, dependency FROM packs"
    >> [&](std::string name, std::string dependency)
    {
        mapPackNameToPackIndex.emplace(name, static_cast<uint32>(packDependencies.size()));

        Vector<uint32> requestIndexes = ConvertStringWithNumbersToVector(dependency);

        packDependencies.emplace_back(std::move(name), std::move(requestIndexes));
    };

    // debug check that max index of fileIndex exist in packIndex
    const auto it = max_element(begin(packIndexes), end(packIndexes));
    const uint32 maxIndex = *it;
    if (maxIndex >= packDependencies.size())
    {
        DAVA_THROW(Exception, "read metadata error - too big index bad meta");
    }

    GenerateDependencyMatrix(numPacks);
}

const PackMetaData::Dependencies& PackMetaData::GetDependencies(uint32 packIndex) const
{
    // all dependent packs with all sub-dependencies
    DVASSERT(packIndex < dependenciesMatrix.size());
    return dependenciesMatrix[packIndex];
}

void PackMetaData::GenerateDependencyMatrixRow(uint32 packIndex, Dependencies& out) const
{
    const String& packName = GetPackInfo(packIndex).packName;
    for (uint32 childPack : GetPackDependencyIndexes(packName))
    {
        out.push_back(childPack);
        GenerateDependencyMatrixRow(childPack, out);
    }
}

void PackMetaData::SortAndEraseDuplicates(Dependencies& c)
{
    // if we are loading from old superpack metadata we have to sort and remove duplicates
    if (!c.empty()
        && (!is_sorted(begin(c), end(c)) || adjacent_find(begin(c), end(c)) != end(c)))
    {
        sort(begin(c), end(c));
        const auto last = unique(begin(c), end(c));
        if (last != end(c))
        {
            c.erase(last, end(c));
        }
    }
}

void PackMetaData::GenerateDependencyMatrix(size_t numPacks)
{
    dependenciesMatrix.clear();
    dependenciesMatrix.resize(numPacks);

    for (size_t packIndex = 0; packIndex < numPacks; ++packIndex)
    {
        Dependencies& depRow = dependenciesMatrix[packIndex];
        GenerateDependencyMatrixRow(static_cast<uint32>(packIndex), depRow);
        SortAndEraseDuplicates(depRow);
    }
}

uint32 PackMetaData::GetPackIndex(const String& requestedPackName) const
{
    const auto it = mapPackNameToPackIndex.find(requestedPackName);
    if (it != end(mapPackNameToPackIndex))
    {
        return it->second;
    }
    DVASSERT(false, "no such pack name");
    DAVA_THROW(Exception, "no such pack name: " + requestedPackName);
}

const Vector<uint32>& PackMetaData::GetPackDependencyIndexes(const String& requestedPackName) const
{
    const PackInfo& packInfo = GetPackInfo(requestedPackName);
    return packInfo.packDependencies;
}

Vector<uint32> PackMetaData::GetFileIndexes(const String& requestedPackName) const
{
    Vector<uint32> result;

    auto it = mapPackNameToPackIndex.find(requestedPackName);
    if (it != end(mapPackNameToPackIndex))
    {
        uint32 packIndex = it->second;
        size_t numFilesInThisPack = std::count(begin(packIndexes), end(packIndexes), packIndex);

        if (numFilesInThisPack > 0)
        {
            result.reserve(numFilesInThisPack);
            uint32 packIndexesSize = static_cast<uint32>(packIndexes.size());
            for (uint32 fileIndex = 0; fileIndex < packIndexesSize; ++fileIndex)
            {
                uint32 index = packIndexes[fileIndex];
                if (index == packIndex)
                {
                    result.push_back(fileIndex);
                }
            }
        }
    }

    return result;
}

uint32 PackMetaData::GetPackIndexForFile(const uint32 fileIndex) const
{
    return packIndexes[fileIndex];
}

const PackMetaData::PackInfo& PackMetaData::GetPackInfo(const uint32 packIndex) const
{
    return packDependencies[packIndex];
}

const PackMetaData::PackInfo& PackMetaData::GetPackInfo(const String& packName) const
{
    auto it = mapPackNameToPackIndex.find(packName);
    if (it != end(mapPackNameToPackIndex))
    {
        const uint32 packIndex = it->second;
        return GetPackInfo(packIndex);
    }

    Logger::Error("error: can't find packName: %s", packName.c_str());
    DVASSERT(false, "debug packName value");
    DAVA_THROW(Exception, "no such packName: " + packName);
}

const FileNamesTree& PackMetaData::GetFileNamesTree() const
{
    return namesTree;
}

Vector<uint8> PackMetaData::Serialize() const
{
    DVASSERT(packDependencies.size() > 0);
    DVASSERT(packIndexes.size() > 0);

    Vector<uint8> compBytes;
    uint32 uncompressedSize = 0;
    {
        std::stringstream ss;

        for (const PackInfo& tuple : packDependencies)
        {
            const String& packName = tuple.packName;
            //const String& depend = tuple.packDependencies;
            ss << packName << ' ';
            for (const uint32& packDepIndex : tuple.packDependencies)
            {
                ss << packDepIndex;
                if (&packDepIndex != &tuple.packDependencies.back())
                {
                    // not last element
                    ss << ", ";
                }
                else
                {
                    // last element no ,
                }
            }
            ss << '\n';
        }

        const String bytes = ss.str();

        uncompressedSize = static_cast<uint32>(bytes.size());

        const Vector<uint8> v(cbegin(bytes), cend(bytes));

        LZ4Compressor compressor;

        if (!compressor.Compress(v, compBytes))
        {
            DAVA_THROW(Exception, "can't compress pack table");
        }
    }

    // FORMAL SPEC of meta info (in future rewrite with http://uscilab.github.io/cereal/)
    // (4b) header - "met2"
    // (4b) num_files
    // (4*num_files) meta_indexes
    // (4b) - uncompressed_size
    // (4b) - compressed_size
    // (compressed_size b) packs_and_dependencies_compressed
    // (4b) - num_packs_with_childs
    // DO
    // (4b) - pack_index
    // (4b) - num_childs
    // (4*num_childs) - dependent_child_indexes
    // WHILE (num_packs_with_childs)

    ScopedPtr<DynamicMemoryFile> file(DynamicMemoryFile::Create(File::READ | File::WRITE));
    if (4 != file->Write("met2", 4))
    {
        DAVA_THROW(Exception, "write meta header failed");
    }

    uint32 numFiles = static_cast<uint32>(packIndexes.size());
    if (4 != file->Write(&numFiles, sizeof(numFiles)))
    {
        DAVA_THROW(Exception, "write num_files failed");
    }

    const uint32 sizeOfFilesMetaIndexes = static_cast<uint32>(packIndexes.size() * sizeof(uint32));
    if (sizeOfFilesMetaIndexes != file->Write(&packIndexes[0], sizeOfFilesMetaIndexes))
    {
        DAVA_THROW(Exception, "write meta file indexes failed");
    }

    if (4 != file->Write(&uncompressedSize, sizeof(uncompressedSize)))
    {
        DAVA_THROW(Exception, "write uncompressedSize failed");
    }

    uint32 compressedSize = static_cast<uint32>(compBytes.size());

    if (4 != file->Write(&compressedSize, sizeof(compressedSize)))
    {
        DAVA_THROW(Exception, "write compressedSize failed");
    }

    if (compressedSize != file->Write(&compBytes[0], compressedSize))
    {
        DAVA_THROW(Exception, "write compressedSize failed");
    }

    // write children table
    uint32 numPacksWithChildren = static_cast<uint32>(count_if(begin(dependenciesMatrix), end(dependenciesMatrix), [](const Dependencies& c)
                                                               {
                                                                   return !c.empty();
                                                               }));

    if (4 != file->Write(&numPacksWithChildren, sizeof(numPacksWithChildren)))
    {
        DAVA_THROW(Exception, "write numPacksWithChildren failed");
    }

    for (uint32 childPackIndex = 0; childPackIndex < dependenciesMatrix.size(); ++childPackIndex)
    {
        const Dependencies& child = dependenciesMatrix[childPackIndex];
        if (!child.empty())
        {
            if (4 != file->Write(&childPackIndex, sizeof(childPackIndex)))
            {
                DAVA_THROW(Exception, "write childPackIndex failed");
            }
            uint32 numChildPacks = static_cast<uint32>(child.size());
            if (4 != file->Write(&numChildPacks, sizeof(numChildPacks)))
            {
                DAVA_THROW(Exception, "write numChildPacks failed");
            }
            const uint32 numBytes = numChildPacks * sizeof(child[0]);
            if (numBytes != file->Write(&child[0], numBytes))
            {
                DAVA_THROW(Exception, "write numBytes failed");
            }
        }
    }

    return file->GetDataVector();
}

struct membuf : std::streambuf
{
    membuf(const void* ptr, size_t size)
    {
        char* begin = const_cast<char*>(static_cast<const char*>(ptr));
        char* end = const_cast<char*>(begin + size);
        setg(begin, begin, end);
    }
};

void PackMetaData::Deserialize(const void* ptr, size_t size)
{
    DVASSERT(ptr != nullptr);
    DVASSERT(size >= 16);

    using namespace std;

    membuf buf(ptr, size);

    istream file(&buf);

    // FORMAL SPEC of meta info (in future rewrite with http://uscilab.github.io/cereal/)
    // (4b) header - "met2"
    // (4b) num_files
    // (4*num_files) meta_indexes
    // (4b) - uncompressed_size
    // (4b) - compressed_size
    // (compressed_size b) packs_and_dependencies_compressed
    // (4b) - num_packs_with_childs
    // DO
    // (4b) - pack_index
    // (4b) - num_childs
    // (4*num_childs) - dependent_child_indexes
    // WHILE (num_packs_with_childs)

    array<char, 4> header;
    file.read(&header[0], 4);
    if (header != array<char, 4>{ 'm', 'e', 't', '2' })
    {
        DAVA_THROW(Exception, "read metadata error - not meta");
    }
    uint32_t numFiles = 0;
    file.read(reinterpret_cast<char*>(&numFiles), 4);
    if (!file)
    {
        DAVA_THROW(Exception, "read metadata error - no numFiles");
    }
    packIndexes.resize(numFiles);

    const uint32_t numFilesBytes = numFiles * 4;
    file.read(reinterpret_cast<char*>(&packIndexes[0]), numFilesBytes);
    if (!file)
    {
        DAVA_THROW(Exception, "read metadata error - no packIndexes");
    }

    uint32_t uncompressedSize = 0;
    file.read(reinterpret_cast<char*>(&uncompressedSize), 4);
    if (!file)
    {
        DAVA_THROW(Exception, "read metadata error - no uncompressedSize");
    }
    uint32_t compressedSize = 0;
    file.read(reinterpret_cast<char*>(&compressedSize), 4);
    if (!file)
    {
        DAVA_THROW(Exception, "read metadata error - no compressedSize");
    }

    vector<uint8_t> compressedBuf(compressedSize);

    file.read(reinterpret_cast<char*>(&compressedBuf[0]), compressedSize);
    if (!file)
    {
        DAVA_THROW(Exception, "read metadata error - no compressedBuf");
    }

    vector<uint8_t> uncompressedBuf(uncompressedSize);

    if (!LZ4Compressor().Decompress(compressedBuf, uncompressedBuf))
    {
        DAVA_THROW(Exception, "read metadata error - can't decompress");
    }

    const char* startBuf = reinterpret_cast<const char*>(&uncompressedBuf[0]);

    membuf outBuf(startBuf, uncompressedSize);
    istream ss(&outBuf);

    // now parse decompressed packs data line by line (%s %s\n) format
    // TODO make parsing without redundant copy of strings
    for (string line, packName, packDependency; getline(ss, line);)
    {
        const auto first_space = line.find(' ');
        if (first_space == string::npos)
        {
            DAVA_THROW(Exception, "can't parse packs and dependencies");
        }
        packName = line.substr(0, first_space);
        packDependency = line.substr(first_space + 1);

        Vector<uint32> packDependencyIndexes = ConvertStringWithNumbersToVector(packDependency);

        uint32 packIndex = static_cast<uint32>(packDependencies.size());
        mapPackNameToPackIndex.emplace(packName, packIndex);
        packDependencies.emplace_back(std::move(packName), std::move(packDependencyIndexes));
    }

    // debug check that max index of fileIndex exist in packIndex
    const auto it = max_element(cbegin(packIndexes), cend(packIndexes));
    const uint32 maxIndex = *it;
    if (maxIndex >= packDependencies.size())
    {
        DAVA_THROW(Exception, "read metadata error - too big index bad meta");
    }

    // read children table
    uint32 numPacksWithChildren = 0;
    file.read(reinterpret_cast<char*>(&numPacksWithChildren), sizeof(numPacksWithChildren));
    if (!file)
    {
        DAVA_THROW(Exception, "read numPacksWithChildren failed");
    }

    dependenciesMatrix.resize(packDependencies.size());

    for (; numPacksWithChildren > 0; --numPacksWithChildren)
    {
        uint32 childPackIndex = 0;
        file.read(reinterpret_cast<char*>(&childPackIndex), sizeof(childPackIndex));
        if (!file)
        {
            DAVA_THROW(Exception, "read childPackIndex failed");
        }

        uint32 numChildPacks = 0;
        file.read(reinterpret_cast<char*>(&numChildPacks), sizeof(numChildPacks));
        if (!file)
        {
            DAVA_THROW(Exception, "read numChildPacks failed");
        }
        Dependencies& dependenciesRow = dependenciesMatrix[childPackIndex];
        dependenciesRow.resize(numChildPacks);

        const uint32 numBytes = numChildPacks * sizeof(dependenciesRow[0]);
        file.read(reinterpret_cast<char*>(&dependenciesRow[0]), numBytes);
        if (!file)
        {
            DAVA_THROW(Exception, "read numBytes failed");
        }

        SortAndEraseDuplicates(dependenciesRow);
    }
}

bool PackMetaData::HasDependency(uint32 packWithDependency, uint32 dependency) const
{
    DVASSERT(packWithDependency < packIndexes.size());
    DVASSERT(dependency < packIndexes.size());

    const Dependencies& dep = dependenciesMatrix[packWithDependency];
    // now we know list of dependencies is sorted and all elements are unique,
    // so we can use binary_search
    return binary_search(begin(dep), end(dep), dependency);
}

bool PackMetaData::HasPack(const String& packName) const
{
    return mapPackNameToPackIndex.find(packName) != end(mapPackNameToPackIndex);
}

} // end namespace DAVA
