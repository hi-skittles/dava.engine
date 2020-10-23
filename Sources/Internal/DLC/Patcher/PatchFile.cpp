#include "PatchFile.h"
#include "FileSystem/File.h"
#include "FileSystem/FileList.h"
#include "FileSystem/FileSystem.h"
#include "Utils/CRC32.h"
#include "Logger/Logger.h"
#include "Concurrency/Thread.h"

namespace DAVA
{
static const char8 davaPatchSignature[] = "[DAVAPATCH]";
static const uint32 davaPatchSignatureSize = sizeof(davaPatchSignature) - 1;

// ======================================================================================
// PatchInfo
// ======================================================================================
PatchInfo::PatchInfo()
{
    Reset();
}

void PatchInfo::Reset()
{
    origSize = 0;
    origCRC = 0;
    origPath = "";

    newSize = 0;
    newCRC = 0;
    newPath = "";
}

bool PatchInfo::Write(File* file)
{
    if (nullptr != file)
    {
        if (!WriteString(file, origPath))
            return false;
        if (sizeof(origSize) != file->Write(&origSize))
            return false;
        if (sizeof(origCRC) != file->Write(&origCRC))
            return false;

        if (!WriteString(file, newPath))
            return false;
        if (sizeof(newSize) != file->Write(&newSize))
            return false;
        if (sizeof(newCRC) != file->Write(&newCRC))
            return false;

        return true;
    }

    return false;
}

bool PatchInfo::Read(File* file)
{
    if (nullptr != file)
    {
        if (!ReadString(file, origPath))
            return false;
        if (sizeof(origSize) != file->Read(&origSize))
            return false;
        if (sizeof(origCRC) != file->Read(&origCRC))
            return false;

        if (!ReadString(file, newPath))
            return false;
        if (sizeof(newSize) != file->Read(&newSize))
            return false;
        if (sizeof(newCRC) != file->Read(&newCRC))
            return false;

        return true;
    }

    return false;
}

bool PatchInfo::WriteString(File* file, const String& str)
{
    bool ret = false;

    uint32 len = static_cast<uint32>(str.length());
    uint32 wlen = file->Write(&len);

    if (wlen == sizeof(uint32))
    {
        wlen = file->Write(str.c_str(), len);
        if (wlen == len)
        {
            ret = true;
        }
    }

    return ret;
}

bool PatchInfo::ReadString(File* file, String& str)
{
    bool ret = false;
    uint32 len = 0;
    size_t rlen = file->Read(&len);

    if (rlen == sizeof(len))
    {
        if (len > 0)
        {
            String tmpStr;
            tmpStr.resize(len);
            rlen = file->Read(&tmpStr[0], static_cast<uint32>(tmpStr.size()));

            if (rlen == tmpStr.size())
            {
                str.swap(tmpStr);
                ret = true;
            }
        }
        else
        {
            str = "";
            ret = true;
        }
    }

    return ret;
}

// ======================================================================================
// PatchFileWriter
// ======================================================================================
PatchFileWriter::PatchFileWriter(const FilePath& path, PatchFileWriter::WriterMode mode, BSType _diffType, bool beVerbose)
    : patchPath(path)
    , diffType(_diffType)
    , verbose(beVerbose)
{
    switch (mode)
    {
    case PatchFileWriter::WRITE:
        FileSystem::Instance()->DeleteFile(patchPath);
        break;
    case PatchFileWriter::APPEND:
        break;
    default:
        DVASSERT(0 && "Unknown PatchWriter mode.");
        break;
    }
}

PatchFileWriter::~PatchFileWriter()
{
}

bool PatchFileWriter::Write(const FilePath& _origBase, const FilePath& origPath, const FilePath& _newBase, const FilePath& newPath)
{
    bool ret = false;

    FilePath origBase = _origBase;
    FilePath newBase = _newBase;

    // if base wasn't set, thread it as current directory
    if (origBase.IsEmpty())
        origBase = FileSystem::Instance()->GetCurrentWorkingDirectory();
    if (newBase.IsEmpty())
        newBase = FileSystem::Instance()->GetCurrentWorkingDirectory();

    // diff between two directories
    if (origPath.IsDirectoryPathname() && newPath.IsDirectoryPathname())
    {
        ret = true;

        List<String> origList;
        List<String> newList;

        EnumerateDir(origPath, origPath, origList);
        EnumerateDir(newPath, newPath, newList);

        // go through files, that should exist in new revision
        for (String& newRelativeFile : newList)
        {
            String origRelativeFile;

            if (std::find(origList.begin(), origList.end(), newRelativeFile) != origList.end())
            {
                origRelativeFile = newRelativeFile;
            }

            FilePath origSinglePath = origPath + origRelativeFile;
            FilePath newSinglePath = newPath + newRelativeFile;

            if (!SingleWrite(origBase, origSinglePath, newBase, newSinglePath))
            {
                printf("Error generating patch:\n\t %s --> %s\n", origPath.GetRelativePathname().c_str(), newPath.GetRelativePathname().c_str());
                ret = false;
                break;
            }
        }

        if (ret)
        {
            // through files, that were in old revision
            // and make patch to delete files that not exist in new revision
            for (String& x : origList)
            {
                if (std::find(newList.begin(), newList.end(), x) == newList.end())
                {
                    FilePath origSinglePath = origPath + x;
                    SingleWrite(origBase, origSinglePath, newBase, FilePath());
                }
            }
        }
    }
    // diff between two file
    else
    {
        ret = SingleWrite(origBase, origPath, newBase, newPath);
    }

    return ret;
}

bool PatchFileWriter::SingleWrite(const FilePath& origBase, const FilePath& origPath, const FilePath& newBase, const FilePath& newPath)
{
    bool ret = false;

    DVASSERT(!origBase.IsEmpty());
    DVASSERT(!newBase.IsEmpty());

    if (FileSystem::Instance()->Exists(origPath) || FileSystem::Instance()->Exists(newPath))
    {
        String origRelativePath = origPath.GetRelativePathname(origBase);
        String newRelativePath = newPath.GetRelativePathname(newBase);

        if (verbose)
        {
            printf("Writing patch\n\t%s ->\n\t%s\n", origPath.GetAbsolutePathname().c_str(), newPath.GetAbsolutePathname().c_str());
        }

        File* patchFile = File::Create(patchPath, File::APPEND | File::WRITE);
        if (nullptr != patchFile)
        {
            PatchInfo patchInfo;

            char8* origData = nullptr;
            char8* newData = nullptr;

            File* origFile = File::Create(origPath, File::OPEN | File::READ);
            if (nullptr != origFile)
            {
                uint32 origSize = static_cast<uint32>(origFile->GetSize());
                origData = new char8[origSize];
                origFile->Read(origData, origSize);

                patchInfo.origPath = origRelativePath;
                patchInfo.origSize = origSize;
                patchInfo.origCRC = CRC32::ForBuffer(origData, origSize);
            }

            File* newFile = File::Create(newPath, File::OPEN | File::READ);
            if (nullptr != newFile)
            {
                uint32 newSize = static_cast<uint32>(newFile->GetSize());
                newData = new char8[newSize];
                newFile->Read(newData, newSize);

                patchInfo.newPath = newRelativePath;
                patchInfo.newSize = newSize;
                patchInfo.newCRC = CRC32::ForBuffer(newData, newSize);
            }

            bool needWriteHeader = false;
            bool needWriteDiff = false;

            // if both files were opened
            if (nullptr != origFile && nullptr != newFile)
            {
                // if there is some diff - write it
                if (patchInfo.origSize != patchInfo.newSize || patchInfo.origCRC != patchInfo.newCRC)
                {
                    needWriteHeader = true;
                    needWriteDiff = true;
                }
                else
                {
                    // files are same, nothing to write
                    ret = true;
                }
            }
            // one or both files weren't opened
            else
            {
                if (nullptr != origFile || nullptr != newFile)
                {
                    needWriteHeader = true;
                    needWriteDiff = true;
                }
                else
                {
                    // diff between directories
                    if ((origPath.IsDirectoryPathname() || origPath.IsEmpty()) &&
                        (newPath.IsDirectoryPathname() || newPath.IsEmpty()))
                    {
                        if (origRelativePath.empty() || newRelativePath.empty())
                        {
                            patchInfo.origPath = origPath.GetRelativePathname(origBase);
                            patchInfo.newPath = newPath.GetRelativePathname(newBase);

                            needWriteHeader = true;
                        }
                        else
                        {
                            ret = true;
                        }
                    }
                }
            }

            if (needWriteHeader)
            {
                uint32 patchSizePos;
                uint32 patchSize = 0;

                // write signature
                patchFile->Write(davaPatchSignature, davaPatchSignatureSize);

                // remember pos were patch size should be written
                patchSizePos = static_cast<uint32>(patchFile->GetPos());

                // write zero patch size. it should be re-writted later
                patchFile->Write(&patchSize);

                // write patch info
                ret = patchInfo.Write(patchFile);

                // write diff, if needed
                if (ret && needWriteDiff)
                {
                    if (!newPath.IsDirectoryPathname() && FileSystem::Instance()->Exists(newPath))
                    {
                        ret = BSDiff::Diff(origData, patchInfo.origSize, newData, patchInfo.newSize, patchFile, diffType);
                    }
                    else
                    {
                        ret = true;
                    }
                }

                // calculate patch size (without signature and patchSize fields)
                patchSize = static_cast<uint32>(patchFile->GetPos()) - patchSizePos - sizeof(patchSize);
                if (ret && patchSize > 0)
                {
                    // seek to pos, where zero patch size was written
                    patchFile->Release();
                    patchFile = File::Create(patchPath, File::OPEN | File::READ | File::WRITE);

                    patchFile->Seek(patchSizePos, File::SEEK_FROM_START);
                    patchFile->Write(&patchSize);
                }

                if (ret && verbose)
                {
                    printf("\tDone, size: %u bytes\n", patchSize);
                }
            }

            SafeRelease(origFile);
            SafeRelease(newFile);
            SafeRelease(patchFile);
            SafeDeleteArray(origData);
            SafeDeleteArray(newData);
        }
    }

    return ret;
}

void PatchFileWriter::EnumerateDir(const FilePath& dirPath, const FilePath& basePath, List<String>& in)
{
    FileList* list = new FileList(dirPath);

    int32 listSize = list->GetCount();
    for (int32 i = 0; i < listSize; ++i)
    {
        String fileName = list->GetFilename(i);
        if (list->IsDirectory(i))
        {
            fileName += '/';
        }

        if (!list->IsNavigationDirectory(i))
        {
            FilePath filePath = FilePath::AddPath(dirPath, fileName);
            if (list->IsDirectory(i))
            {
                EnumerateDir(filePath, basePath, in);
            }

            in.push_back(filePath.GetRelativePathname(basePath));
        }
    }

    list->Release();
}

// ======================================================================================
// PatchFileReader
// ======================================================================================
PatchFileReader::PatchFileReader(const FilePath& path, bool beVerbose, bool enablePermissive)
    : isPermissiveMode(enablePermissive)
    , lastError(ERROR_NO)
    , parseError(ERROR_NO)
    , lastFileErrno(0)
    , verbose(beVerbose)
    , eof(true)
    , initialPositionsCount(0)
    , curPatchIndex(0)
    , curBSDiffPos(0)
{
    patchFile = File::Create(path, File::OPEN | File::READ | File::WRITE);
    if (nullptr != patchFile)
    {
        char8 signature[davaPatchSignatureSize];
        uint32 patchFileSize = static_cast<uint32>(patchFile->GetSize());

        while (true)
        {
            signature[0] = 0;

            // read signature and patch size
            uint32 sigReadSize = patchFile->Read(signature, davaPatchSignatureSize);

            // if nothing was read, this should be normal end of file
            // so we can break loop without errors
            if (0 == sigReadSize && patchFile->IsEof())
            {
                break;
            }

            // check for signature match
            if (sigReadSize != davaPatchSignatureSize ||
                Memcmp(signature, davaPatchSignature, davaPatchSignatureSize) != 0)
            {
                // wrong signature, we should break with error
                parseError = ERROR_CORRUPTED;
                Logger::ErrorToFile(logFilePath, "[PatchFileReader::PatchFileReader] Check for signature match failed");
                break;
            }

            // now read size of patch
            uint32 patchSize = 0;
            uint32 szReadSize = patchFile->Read(&patchSize);

            // check for correct patch size
            if (sizeof(patchSize) != szReadSize || 0 == patchSize)
            {
                // wrong patch size or read operation failed
                parseError = ERROR_CORRUPTED;
                Logger::ErrorToFile(logFilePath, "[PatchFileReader::PatchFileReader]  Wrong patch size or read operation failed");
                break;
            }

            // remember current file pos
            uint32 curPos = static_cast<uint32>(patchFile->GetPos());

            // check if next patch position isn't out of file size
            if ((curPos + patchSize) > patchFileSize)
            {
                parseError = ERROR_CORRUPTED;
                Logger::ErrorToFile(logFilePath, "[PatchFileReader::PatchFileReader]  Next patch position is out of file size");
                break;
            }

            // store remembered patch pos in our internal list
            // and go to the next patch chunk
            patchPositions.push_back(curPos);
            patchFile->Seek(patchSize, File::SEEK_FROM_CURRENT);
        }

        initialPositionsCount = patchPositions.size();
    }
    else
    {
        parseError = ERROR_CANT_READ;
        lastFileErrno = errno;
        lastErrorDetails.expected.path = path;
        lastErrorDetails.actual.path = "";
    }
}

PatchFileReader::~PatchFileReader()
{
    SafeRelease(patchFile);
}

bool PatchFileReader::ReadFirst()
{
    if (patchPositions.size() > 0)
    {
        curPatchIndex = 0;
        eof = false;
    }
    else
    {
        eof = true;
    }

    return DoRead();
}

bool PatchFileReader::ReadNext()
{
    if (patchPositions.size() > 0 && curPatchIndex < (patchPositions.size() - 1))
    {
        curPatchIndex++;
    }
    else
    {
        eof = true;
    }

    return DoRead();
}

bool PatchFileReader::ReadLast()
{
    if (patchPositions.size() > 0)
    {
        curPatchIndex = patchPositions.size() - 1;
        eof = false;
    }
    else
    {
        eof = true;
    }

    return DoRead();
}

bool PatchFileReader::ReadPrev()
{
    if (curPatchIndex > 0 && curPatchIndex <= patchPositions.size())
    {
        curPatchIndex--;
    }
    else
    {
        eof = true;
    }

    return DoRead();
}

const PatchInfo* PatchFileReader::GetCurInfo() const
{
    const PatchInfo* ret = nullptr;

    if (curBSDiffPos > 0)
    {
        ret = &curInfo;
    }

    return ret;
}

bool PatchFileReader::DoRead()
{
    lastErrorDetails = PatchingErrorDetails();
    bool ret = false;
    lastError = ERROR_NO;
    lastFileErrno = 0;

    // reset header and bsdiff pos
    curBSDiffPos = 0;
    curInfo.Reset();

    if (nullptr != patchFile)
    {
        if (!eof)
        {
            // read header and remember bsdiff pos
            int32 patchPos = patchPositions[curPatchIndex];
            if (patchFile->Seek(patchPos, File::SEEK_FROM_START) && curInfo.Read(patchFile))
            {
                curBSDiffPos = static_cast<uint32>(patchFile->GetPos());
                ret = true;
            }
            else
            {
                lastError = ERROR_CORRUPTED;
            }
        }
        else
        {
            // end of file, so we should set lastError to
            // the parseError value
            lastError = parseError;

            // no errors, check if patch file wasn't empty
            if (ERROR_NO == lastError && 0 == initialPositionsCount)
            {
                // patch file without
                lastError = ERROR_EMPTY_PATCH;
            }
        }
    }
    else
    {
        lastError = parseError;
    }

    return ret;
}

bool PatchFileReader::Truncate()
{
    bool ret = false;

    if (nullptr != patchFile && ERROR_NO == parseError && !eof && curPatchIndex < patchPositions.size())
    {
        int32 patchPos = patchPositions[curPatchIndex];
        int32 signarutePos = patchPos - davaPatchSignatureSize - sizeof(uint32);
        if (signarutePos >= 0)
        {
            if (patchFile->Truncate(signarutePos))
            {
                curBSDiffPos = 0;
                patchPositions.resize(curPatchIndex);
                ret = true;
            }
        }
    }

    return ret;
}

bool PatchFileReader::Apply(const FilePath& _origBase, const FilePath& _origPath, const FilePath& _newBase, const FilePath& _newPath)
{
    bool ret = true;
    lastError = ERROR_NO;
    lastFileErrno = 0;
    lastErrorDetails = PatchingErrorDetails();

    FilePath origBase = _origBase;
    FilePath newBase = _newBase;
    FilePath origPath = _origPath;
    FilePath newPath = _newPath;

    // if base wasn't set, thread it as current directory
    if (origBase.IsEmpty())
        origBase = FileSystem::Instance()->GetCurrentWorkingDirectory();
    if (newBase.IsEmpty())
        newBase = FileSystem::Instance()->GetCurrentWorkingDirectory();

    // if path wasn't set, generate it using base dir and info about current patch
    if (origPath.IsEmpty())
        origPath = origBase + curInfo.origPath;
    if (newPath.IsEmpty())
        newPath = newBase + curInfo.newPath;

    if (0 == curBSDiffPos)
    {
        lastError = ERROR_EMPTY_PATCH;
        Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply]  Patch is empty");
        return false;
    }

    if (verbose)
    {
        Logger::InfoToFile(logFilePath, "[PatchFileReader::Apply] Applying patch #%u\n\t%s, sz = %u ->\n\t%s, sz = %u\n", curPatchIndex, origPath.GetAbsolutePathname().c_str(), curInfo.origSize, newPath.GetAbsolutePathname().c_str(), curInfo.newSize);
    }
    else
    {
        Logger::InfoToFile(logFilePath, "[PatchFileReader::Apply] Applying patch #%u\n", curPatchIndex);
    }

    if (!curInfo.origPath.empty() && !curInfo.newPath.empty() &&
        curInfo.origSize == curInfo.newSize && curInfo.origCRC == curInfo.newCRC)
    {
        // orig file not changed
        if (origPath != newPath)
        {
            if (!FileSystem::Instance()->CopyFile(origPath, newPath, true))
            {
                // can't copy original file to the new path.
                lastError = ERROR_NEW_WRITE;
                lastFileErrno = errno;
                lastErrorDetails.expected.path = newPath;
                lastErrorDetails.actual.path = origPath;

                ret = false;
                Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't copy %s to %s", origPath.GetFilename().c_str(), newPath.GetFilename().c_str());
            }
        }
    }
    else
    {
        char8* origData = nullptr;
        char8* newData = nullptr;

        // if new file should exist after patching
        if (!curInfo.newPath.empty())
        {
            // if there is already exist some
            // we should check if it is already patched
            if (FileSystem::Instance()->Exists(newPath))
            {
                File* checkFile = File::Create(newPath, File::OPEN | File::READ);
                if (nullptr != checkFile)
                {
                    uint32 checkCRC = CRC32::ForFile(newPath);
                    uint32 checkSize = static_cast<uint32>(checkFile->GetSize());
                    SafeRelease(checkFile);

                    // file exists, so check it size and CRC
                    if (checkSize == curInfo.newSize && checkCRC == curInfo.newCRC)
                    {
                        return true;
                    }
                }
            }

            // should original file exists?
            if (!curInfo.origPath.empty())
            {
                File* origFile = File::Create(origPath, File::OPEN | File::READ);
                if (nullptr == origFile)
                {
                    lastFileErrno = errno;
                    lastErrorDetails.expected.path = origPath;
                    lastErrorDetails.actual.path = "";
                    lastError = ERROR_ORIG_READ;
                    Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't read origFile from %s", origPath.GetAbsolutePathname().c_str());
                    ret = false;
                }
                else
                {
                    uint32 origSize = static_cast<uint32>(origFile->GetSize());
                    origData = new (std::nothrow) char8[origSize];

                    if (nullptr != origData)
                    {
                        const uint32 bytesReaded = origFile->Read(origData, origSize);
                        if (bytesReaded != origSize)
                        {
                            lastFileErrno = errno;
                            lastErrorDetails.actual.size = bytesReaded;
                            lastErrorDetails.expected.size = origSize;
                            lastErrorDetails.actual.path = origPath;
                            lastErrorDetails.expected.path = origPath;
                            lastError = ERROR_ORIG_READ;
                            Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply]  %d bytes readed but %d expected from %s.", bytesReaded, origSize, origPath.GetAbsolutePathname().c_str());
                            ret = false;
                        }

                        origFile->Release();

                        // even if ret == false we need to provide CRC info.
                        uint32 origCRC = CRC32::ForBuffer(origData, bytesReaded);
                        if (origSize != curInfo.origSize || origCRC != curInfo.origCRC)
                        {
                            lastErrorDetails.actual.size = bytesReaded;
                            lastErrorDetails.expected.size = curInfo.origSize;
                            lastErrorDetails.actual.crc = origCRC;
                            lastErrorDetails.expected.crc = curInfo.origCRC;
                            lastErrorDetails.actual.path = origPath;
                            lastErrorDetails.expected.path = curInfo.origPath;

                            // source crc differ for expected
                            lastError = ERROR_ORIG_BUFFER_CRC;
                            ret = false;
                            Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Crc is not match for file %s", origPath.GetAbsolutePathname().c_str());
                        }
                    }
                    else
                    {
                        // can't allocate memory
                        lastError = ERROR_MEMORY;
                        ret = false;
                        Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Cant'allocate %d bytes for %s", origSize, origPath.GetAbsolutePathname().c_str());
                    }
                }
            }

            if (ret)
            {
                // is this patch for directory creation?
                if (newPath.IsDirectoryPathname() && curInfo.newSize == 0 && curInfo.newCRC == 0)
                {
                    if (FileSystem::DIRECTORY_CANT_CREATE == FileSystem::Instance()->CreateDirectory(newPath, true))
                    {
                        lastFileErrno = errno;
                        lastErrorDetails.expected.path = newPath;
                        lastErrorDetails.actual.path = "";
                        lastError = ERROR_NEW_CREATE;
                        ret = false;
                        Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't create new directory %s", newPath.GetAbsolutePathname().c_str());
                    }
                }
                // for file patching or creation
                else
                {
                    FilePath tmpNewPath = newPath;
                    tmpNewPath += String(".tmp_patch");

                    // create folders if needed
                    FileSystem::Instance()->CreateDirectory(tmpNewPath.GetDirectory(), true);

                    File* newFile = File::Create(tmpNewPath, File::CREATE | File::WRITE);
                    if (nullptr == newFile)
                    {
                        lastFileErrno = errno;
                        lastErrorDetails.expected.path = tmpNewPath;
                        lastErrorDetails.actual.path = "";
                        lastError = ERROR_NEW_CREATE;
                        ret = false;
                        Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't create temp file &s", tmpNewPath.GetAbsolutePathname().c_str());
                    }
                    else
                    {
                        if (curInfo.newSize > 0)
                        {
                            newData = new (std::nothrow) char8[curInfo.newSize];

                            if (nullptr != newData)
                            {
                                if (BSDiff::Patch(origData, curInfo.origSize, newData, curInfo.newSize, patchFile))
                                {
                                    if (curInfo.newSize != newFile->Write(newData, curInfo.newSize))
                                    {
                                        ret = false;
                                        Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't write data to file %s", newFile->GetFilename().GetAbsolutePathname().c_str());
                                    }
                                    else
                                    {
                                        if (!newFile->Flush())
                                        {
                                            ret = false;
                                            Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] can't flush newFile. %s", tmpNewPath.GetAbsolutePathname().c_str());
                                        }
                                    }

                                    if (!ret)
                                    {
                                        lastFileErrno = errno;
                                        lastErrorDetails.expected.path = tmpNewPath;
                                        lastErrorDetails.expected.size = curInfo.newSize;
                                        lastErrorDetails.actual.path = "";
                                        lastErrorDetails.actual.size = static_cast<uint32>(newFile->GetSize());
                                        lastError = ERROR_NEW_WRITE;
                                    }
                                }
                                else
                                {
                                    lastError = ERROR_CORRUPTED;
                                    ret = false;
                                    Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't patch %s", origPath.GetAbsolutePathname().c_str());
                                }
                            }
                            else
                            {
                                // can't allocate memory
                                lastError = ERROR_MEMORY;
                                ret = false;
                                Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't allocate %d bytes for new data", curInfo.newSize);
                            }
                        }

                        newFile->Release();

                        // if no errors - check for new file CRC
                        if (ret)
                        {
                            size_t counter = 0;
                            uint32 actualCRC = 0;
                            uint32 actualSizeFromFile = 0;
                            do
                            {
                                bool canContinue = true;

                                ScopedPtr<File> justWrittenFile(File::Create(tmpNewPath, File::OPEN | File::READ));
                                if (!justWrittenFile)
                                {
                                    Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] can't open just written file: %s", tmpNewPath.GetAbsolutePathname().c_str());
                                    canContinue = false;
                                }

                                if (canContinue)
                                {
                                    actualSizeFromFile = static_cast<uint32>(justWrittenFile->GetSize());
                                    if (actualSizeFromFile != curInfo.newSize)
                                    {
                                        Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] file size not match: %d != %d ", actualSizeFromFile, curInfo.newSize);
                                        canContinue = false;
                                    }
                                }

                                Vector<char> content;

                                if (canContinue)
                                {
                                    content.resize(actualSizeFromFile);
                                    uint32 bytesRead = justWrittenFile->Read(content.data(), static_cast<uint32>(content.size()));
                                    if (bytesRead != curInfo.newSize)
                                    {
                                        Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] read size not match: %d != %d ", bytesRead, curInfo.newSize);
                                        canContinue = false;
                                    }
                                }

                                if (canContinue)
                                {
                                    actualCRC = CRC32::ForBuffer(content.data(), static_cast<uint32>(content.size()));
                                    if (curInfo.newCRC != actualCRC)
                                    {
                                        Logger::ErrorToFile(logFilePath,
                                                            "[PatchFileReader::Apply] New file crc32 doesn't matches to expected."
                                                            " %s counter=%d actual_crc=0x%X expected_crc=0x%X, actual_size=%d expected_size=%d",
                                                            tmpNewPath.GetAbsolutePathname().c_str(),
                                                            counter, actualCRC,
                                                            curInfo.newCRC,
                                                            actualSizeFromFile,
                                                            curInfo.newSize);
                                        canContinue = false;
                                    }
                                }

                                if (!canContinue)
                                {
                                    // wait a little bit, let OS to update file
                                    Thread::Sleep(500);
                                }
                            } while (++counter < 10 && curInfo.newCRC != actualCRC);

                            if (curInfo.newCRC != actualCRC)
                            {
                                lastErrorDetails.actual.path = tmpNewPath;
                                lastErrorDetails.expected.path = tmpNewPath;
                                lastErrorDetails.actual.crc = actualCRC;
                                lastErrorDetails.expected.crc = curInfo.newCRC;
                                lastErrorDetails.actual.size = actualSizeFromFile;
                                lastErrorDetails.expected.size = curInfo.newSize;
                                lastError = ERROR_NEW_CRC;
                                ret = false;

                                Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] New file crc doesn't matches to expected."
                                                                 " %s counter=%d actual_crc=0x%X expected_crc=0x%X, actual_size=%d expected_size=%d",
                                                    tmpNewPath.GetAbsolutePathname().c_str(), counter, actualCRC, curInfo.newCRC, actualSizeFromFile, curInfo.newSize);
                                DVASSERT(false, "check log now!");
                            }
                        }

                        // if no errors - move tmp file into destination path
                        if (ret)
                        {
                            // this operation should be atomic
                            if (!FileSystem::Instance()->MoveFile(tmpNewPath, newPath, true))
                            {
                                lastFileErrno = errno;
                                lastErrorDetails.expected.path = newPath;
                                lastErrorDetails.actual.path = tmpNewPath;
                                lastError = ERROR_NEW_WRITE;
                                ret = false;
                                Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't move patched file to %s", newPath.GetAbsolutePathname().c_str());

                                FileSystem::Instance()->DeleteFile(tmpNewPath);
                            }
                        }
                    }
                }
            }

            SafeDeleteArray(origData);
            SafeDeleteArray(newData);
        }
        // there should be no new file after patching
        else
        {
            // should original file exists?
            // PermissiveMode allows to delete changed files if they should be deleted
            if (!isPermissiveMode && !curInfo.origPath.empty())
            {
                // check original crc
                uint32 origCRC = CRC32::ForFile(origPath);
                if (origCRC != curInfo.origCRC)
                {
                    lastErrorDetails.actual.crc = origCRC;
                    lastErrorDetails.expected.crc = curInfo.origCRC;
                    lastErrorDetails.actual.path = origPath;
                    lastErrorDetails.expected.path = curInfo.origPath;
                    lastError = ERROR_ORIG_FILE_CRC;
                    ret = false;
                    Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Original crc doesn't matches to expected. %s", origPath.GetAbsolutePathname().c_str());
                }
            }

            if (ret)
            {
                // check if file exists in new basePath
                FilePath newPathToDelete = newPath;
                if (newPathToDelete.IsDirectoryPathname())
                {
                    newPathToDelete += curInfo.origPath;
                }

                // don't try to delete file or folder if it is not exists in permissive mode
                // try to delete anyway if permissive mode is not enabled
                bool isFileExists = FileSystem::Instance()->Exists(newPathToDelete);
                bool shouldToTtryToDelete = !isPermissiveMode || isFileExists;
                if (shouldToTtryToDelete)
                {
                    if (newPathToDelete.IsDirectoryPathname())
                    {
                        // delete only empty directory
                        if (!FileSystem::Instance()->DeleteDirectory(newPathToDelete))
                        {
                            lastFileErrno = errno;
                            lastErrorDetails.actual.path = newPathToDelete;
                            lastErrorDetails.expected.path = "";
                            lastError = ERROR_NEW_WRITE;
                            ret = false;
                            Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't delete %s", newPathToDelete.GetAbsolutePathname().c_str());
                        }
                    }
                    else
                    {
                        if (!FileSystem::Instance()->DeleteFile(newPathToDelete))
                        {
                            lastFileErrno = errno;
                            lastErrorDetails.actual.path = newPathToDelete;
                            lastErrorDetails.expected.path = "";
                            lastError = ERROR_NEW_WRITE;
                            ret = false;
                            Logger::ErrorToFile(logFilePath, "[PatchFileReader::Apply] Can't delete %s", newPathToDelete.GetAbsolutePathname().c_str());
                        }
                    }
                }
            }
        }
    }

    if (ret && verbose)
    {
        Logger::InfoToFile(logFilePath, "[PatchFileReader::Apply] Done!\n");
    }

    return ret;
}
}
