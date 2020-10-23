#include "DLCDownloaderDefaultWriter.h"

#include "Engine/Engine.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"

namespace DAVA
{
DLCDownloaderDefaultWriter::DLCDownloaderDefaultWriter(const String& outputFile)
{
    FileSystem* fs = GetEngineContext()->fileSystem;

    FilePath path = outputFile;
    FilePath directory = path.GetDirectory();
    if (FileSystem::DIRECTORY_CANT_CREATE == fs->CreateDirectory(directory, true))
    {
        const char* err = strerror(errno);
        DAVA_THROW(Exception, "can't create output directory: " + directory.GetAbsolutePathname() + " errno:(" + err + ") outputFile: " + outputFile);
    }

    f = RefPtr<File>(File::Create(outputFile, File::WRITE | File::APPEND));

    if (!f)
    {
        StringStream ss;
        ss << "can't create output file: " << outputFile << " errno(" << errno << ") " << strerror(errno);
        DAVA_THROW(Exception, ss.str());
    }
}

DLCDownloaderDefaultWriter::~DLCDownloaderDefaultWriter() = default;

void DLCDownloaderDefaultWriter::MoveToEndOfFile() const
{
    bool result = f->Seek(0, File::eFileSeek::SEEK_FROM_END);
    DVASSERT(result);
}

// save next buffer bytes into memory or file
uint64 DLCDownloaderDefaultWriter::Save(const void* ptr, uint64 size)
{
    return f->Write(ptr, static_cast<uint32>(size));
}
// return current size of saved byte stream
uint64 DLCDownloaderDefaultWriter::GetSeekPos()
{
    return f->GetPos();
}

bool DLCDownloaderDefaultWriter::Truncate()
{
    return f->Truncate(0);
}

bool DLCDownloaderDefaultWriter::Close()
{
    const bool result = f->Flush();
    f.Set(nullptr);
    return result;
}

bool DLCDownloaderDefaultWriter::IsClosed() const
{
    return f == nullptr;
}
}
