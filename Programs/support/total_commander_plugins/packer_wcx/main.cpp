#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#include "archive.h"

using DWORD = std::uint32_t;
const std::uint32_t MAX_PATH = 260;

#if defined(__MINGW32__)
#define DLL_EXPORT __declspec(dllexport)
#define STDCALL __stdcall
#else /* defined (_WIN32) */
#define DLL_EXPORT
#define STDCALL
#endif

#include "wcxhead.h"

using HANDLE = void*;

extern "C" {
DLL_EXPORT HANDLE STDCALL
OpenArchive(tOpenArchiveData* ArchiveData);
DLL_EXPORT int STDCALL
ReadHeader(HANDLE hArcData, tHeaderData* HeaderData);
DLL_EXPORT int STDCALL
ProcessFile(HANDLE hArcData, int Operation, char* DestPath, char* DestName);
DLL_EXPORT int STDCALL CloseArchive(HANDLE hArcData);
DLL_EXPORT void STDCALL
SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1);
DLL_EXPORT void STDCALL
SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc);
DLL_EXPORT int STDCALL GetPackerCaps();
}

std::ofstream l;

HANDLE STDCALL OpenArchive(tOpenArchiveData* ArchiveData)
{
    if (!l.is_open())
    {
        const char* logger_path = std::getenv("DVPK_PLUGIN_LOG");
        if (logger_path != nullptr)
        {
            l.open(logger_path);
        }
    }

    Archive* archive = nullptr;
    try
    {
        l << "begin open archive: " << ArchiveData->ArcName << '\n';

        archive = Create(ArchiveData->ArcName);

        l << "open archive: " << ArchiveData->ArcName << '\n';
    }
    catch (std::exception& ex)
    {
        l << ex.what() << std::flush;
        ArchiveData->OpenResult = E_BAD_ARCHIVE;
    }
    return archive;
}

int STDCALL CloseArchive(HANDLE hArcData)
{
    Archive* archive = reinterpret_cast<Archive*>(hArcData);

    l << "close archive: " << archive->archive_name << '\n';

    delete archive;
    return 0;
}

// called multiple times till return 0 on finish files
int STDCALL ReadHeader(HANDLE hArcData, tHeaderData* HeaderData)
{
    l << "read header\n";

    Archive* archive = reinterpret_cast<Archive*>(hArcData);

    const std::vector<pack_format::file_info>& files = archive->GetFilesInfo();
    if (archive->file_index < files.size())
    {
        const pack_format::file_info& info = files.at(archive->file_index);

        std::string name(info.relativeFilePath);

#if defined(__MINGW32__) || defined(_MSVC)
        std::for_each(begin(name), end(name), [](char& ch) {
            if (ch == '/')
            {
                ch = '\\';
            }
        });
#endif

        HeaderData->FileAttr = 1; // FILE_SHARE_READ;
        std::strncpy(HeaderData->FileName, name.c_str(), MAX_PATH);
        std::strncpy(HeaderData->ArcName, archive->archive_name.c_str(), MAX_PATH);

        HeaderData->FileCRC = info.hash;
        HeaderData->FileTime = 0;
        HeaderData->UnpSize = info.originalSize;
        HeaderData->PackSize = info.compressedSize;

        archive->last_file_name = info.relativeFilePath;

        l << "read header file: " << HeaderData->FileName << " relative: "
          << info.relativeFilePath << " index: " << archive->file_index << '\n';

        ++archive->file_index;
    }
    else
    {
        if (archive->file_index == files.size() && archive->HasMeta())
        {
            ++archive->file_index;
            std::memset(HeaderData, 0, sizeof(*HeaderData));

            std::strncpy(HeaderData->FileName, "meta.meta", MAX_PATH);
            std::strncpy(HeaderData->ArcName, archive->archive_name.c_str(), MAX_PATH);
            HeaderData->PackSize = 0;
            auto& meta = archive->GetMeta();
            HeaderData->UnpSize = meta.get_num_packs();

            archive->last_file_name = "meta.meta";

            l << "add meta file\n";
            return 0;
        }
        else
        {
            archive->file_index = 0;
            std::memset(HeaderData, 0, sizeof(*HeaderData));

            l << "end header\n";
            return E_END_ARCHIVE;
        }
    }

    return 0;
}

int STDCALL ProcessFile(HANDLE hArcData, int Operation, char* DestPath,
                        char* DestName)
{
    l << "process_file\n";

    if (PK_SKIP == Operation)
    {
    }
    else if (PK_TEST == Operation)
    {
    }
    else if (PK_EXTRACT == Operation)
    {
        Archive* archive = reinterpret_cast<Archive*>(hArcData);
        if (archive->last_file_name == std::string("meta.meta"))
        {
            std::string data = archive->PrintMeta();
            std::string outputName = DestName ? DestName : DestPath;

            std::ofstream out(outputName, std::ios_base::binary);
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
        else if (archive->HasFile(archive->last_file_name))
        {
            std::vector<uint8_t> data;
            archive->HoadFile(archive->last_file_name, data);
            std::string outputName = DestName ? DestName : DestPath;

            std::ofstream out(outputName, std::ios_base::binary);
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    }
    return 0;
}

void STDCALL
SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
}

int STDCALL GetPackerCaps()
{
    return PK_CAPS_MULTIPLE; // | PK_CAPS_HIDE;
}

void STDCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
    // do nothing
}
