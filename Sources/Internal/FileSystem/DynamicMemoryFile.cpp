#include "FileSystem/DynamicMemoryFile.h"
#include "Utils/StringFormat.h"
#include "Logger/Logger.h"

namespace DAVA
{
DynamicMemoryFile* DynamicMemoryFile::Create(Vector<uint8>&& data, uint32 attributes, const FilePath& name)
{
    DynamicMemoryFile* f = new DynamicMemoryFile();
    f->data = std::move(data);
    f->currentPtr = 0;
    f->fileAttributes = attributes;
    f->filename = name;
    return f;
}

DynamicMemoryFile* DynamicMemoryFile::Create(const uint8* data, int32 dataSize, uint32 attributes)
{
    DynamicMemoryFile* fl = new DynamicMemoryFile();
    fl->filename = Format("memory_file_%p", static_cast<void*>(fl));
    fl->Write(data, dataSize);
    fl->fileAttributes = attributes;
    fl->currentPtr = 0;

    return fl;
}

DynamicMemoryFile* DynamicMemoryFile::Create(uint32 attributes)
{
    DynamicMemoryFile* fl = new DynamicMemoryFile();
    fl->fileAttributes = attributes;
    fl->filename = Format("memory_file_%p", static_cast<void*>(fl));

    return fl;
}

DynamicMemoryFile::DynamicMemoryFile()
    : currentPtr(0)
    , fileAttributes(WRITE)
{
}

DynamicMemoryFile::DynamicMemoryFile(DynamicMemoryFile&& other) noexcept
: currentPtr(other.currentPtr)
  ,
  data(std::move(other.data))
  ,
  fileAttributes(other.fileAttributes)
{
    other.fileAttributes = 0;
    other.currentPtr = 0;
}

DynamicMemoryFile& DynamicMemoryFile::operator=(DynamicMemoryFile&& other) noexcept
{
    currentPtr = other.currentPtr;
    data = std::move(other.data);
    fileAttributes = other.fileAttributes;

    other.fileAttributes = 0;
    other.currentPtr = 0;

    return *this;
}

const uint8* DynamicMemoryFile::GetData() const
{
    return data.data();
}

uint32 DynamicMemoryFile::Write(const void* pointerToData, uint32 dataSize)
{
    DVASSERT(nullptr != pointerToData);

    if (!(fileAttributes & WRITE) && !(fileAttributes & APPEND))
    {
        return 0;
    }

    if (data.size() < currentPtr + dataSize)
    {
        data.resize(static_cast<size_t>(currentPtr + dataSize));
    }
    if (dataSize)
    {
        Memcpy(&(data[static_cast<size_t>(currentPtr)]), pointerToData, dataSize);
        currentPtr += dataSize;
    }

    return dataSize;
}

uint32 DynamicMemoryFile::Read(void* pointerToData, uint32 dataSize)
{
    DVASSERT(NULL != pointerToData);

    if (!(fileAttributes & READ))
    {
        Logger::Error("memory_file read failed: 0(expected: %u) bytes from file: %s, errno: %s",
                      dataSize, filename.GetStringValue().c_str(), std::strerror(EACCES));
        return 0;
    }

    if (currentPtr == data.size() && !isEof && dataSize > 0)
    {
        isEof = true;
        return 0;
    }

    int32 realReadSize = static_cast<int32>(dataSize);
    const auto size = static_cast<uint32>(data.size());
    if (currentPtr + realReadSize > size)
    {
        isEof = true;
        realReadSize = size - static_cast<uint32>(currentPtr);
    }
    if (realReadSize > 0)
    {
        Memcpy(pointerToData, &(data[static_cast<size_t>(currentPtr)]), realReadSize);
        currentPtr += realReadSize;

        return realReadSize;
    }

    return 0;
}

uint64 DynamicMemoryFile::GetPos() const
{
    return currentPtr;
}

uint64 DynamicMemoryFile::GetSize() const
{
    return static_cast<uint64>(data.size());
}

bool DynamicMemoryFile::Seek(int64 position, eFileSeek seekType)
{
    int64 pos = 0;
    switch (seekType)
    {
    case SEEK_FROM_START:
        pos = position;
        break;
    case SEEK_FROM_CURRENT:
        pos = GetPos() + position;
        break;
    case SEEK_FROM_END:
        pos = GetSize() - 1 + position;
        break;
    default:
        return false;
    };

    if (pos < 0)
    {
        return false;
    }

    if (pos > static_cast<int64>(data.size()) && !(fileAttributes & WRITE))
    {
        Logger::Warning("memory_file opened in readonly mode you about to seek over EOF (POSIX let it)");
    }

    currentPtr = pos;
    // behavior like in std::FILE http://en.cppreference.com/w/c/io/fseek
    isEof = false;

    return true;
}

bool DynamicMemoryFile::IsEof() const
{
    return isEof;
}

bool DynamicMemoryFile::Truncate(uint64 size)
{
    if (!(fileAttributes & WRITE))
    {
        return false;
    }

    data.resize(size_t(size));
    currentPtr = Min(currentPtr, size);
    isEof = currentPtr == size;

    return true;
}

bool DynamicMemoryFile::Flush()
{
    return true;
}
};
