#pragma once

#include "Base/BaseTypes.h"
#include "Functional/Function.h"

namespace DAVA
{
class FilePath;

class FileNamesTree
{
public:
    FileNamesTree() = default;
    void Add(const String& relativeFilePath);
    bool Find(const String& relativeFilePath) const;

private:
    static Vector<String> GetPathElements(const String& relativeFilePath);

    struct Node
    {
        Map<String, Node> children;
    };

    Node treeRoot;
};

class PackMetaData
{
public:
    /** Create meta from sqlite db file
	    open DB and read meta data vector
		Throw DAVA::Exception on error
		*/
    explicit PackMetaData(const FilePath& metaDb);
    /** Create meta from serialized bytes
		    Throw exception on error
		*/
    PackMetaData(const void* ptr, std::size_t size, const String& fileNames);

    uint32 GetPackIndex(const String& requestedPackName) const;

    const Vector<uint32>& GetPackDependencyIndexes(const String& requestedPackName) const;

    Vector<uint32> GetFileIndexes(const String& requestedPackName) const;

    uint32 GetPackIndexForFile(const uint32 fileIndex) const;

    size_t GetFileCount() const;

    size_t GetPacksCount() const;

    struct PackInfo
    {
        PackInfo(String n, Vector<uint32> dep)
            : packName(std::move(n))
            , packDependencies(std::move(dep))
        {
        }
        String packName;
        Vector<uint32> packDependencies;
    };

    const PackInfo& GetPackInfo(const uint32 packIndex) const;
    const PackInfo& GetPackInfo(const String& packName) const;

    const FileNamesTree& GetFileNamesTree() const;

    Vector<uint8> Serialize() const;
    void Deserialize(const void* ptr, size_t size);

    bool HasDependency(uint32 packWithDependency, uint32 dependency) const;

    bool HasPack(const String& packName) const;
    /**
        Sorted vector of unique pack indexes, includes full list of all dependent packs indexes
     */
    using Dependencies = Vector<uint32>;

    const Dependencies& GetDependencies(uint32 packIndex) const;

private:
    void GenerateDependencyMatrix(size_t numPacks);
    void GenerateDependencyMatrixRow(uint32 packIndex, Dependencies& out) const;
    static void SortAndEraseDuplicates(Dependencies& c);
    Vector<uint32> ConvertStringWithNumbersToVector(const String& str) const;
    // fileNames already in DVPK format
    // table 1.
    // fileName -> fileIndex(0-NUM_FILES) -> packIndex(0-NUM_PACKS)
    Vector<uint32> packIndexes;
    // table 2.
    // packIndex(0-NUM_PACKS) -> packName, dependencies
    Vector<PackInfo> packDependencies;

    // packIndex(0-NUM_PACKS) -> Vector of child indexes
    Vector<Dependencies> dependenciesMatrix;

    // packName -> packIndex (auto generated during deserializing)
    UnorderedMap<String, uint32> mapPackNameToPackIndex;

    // all file names to check if file belong to this meta
    FileNamesTree namesTree;
};

inline size_t PackMetaData::GetFileCount() const
{
    return packIndexes.size();
}

inline size_t PackMetaData::GetPacksCount() const
{
    return packDependencies.size();
}

} // end namespace DAVA
