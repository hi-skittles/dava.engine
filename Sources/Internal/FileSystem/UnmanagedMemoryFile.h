#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/File.h"

namespace DAVA
{
/**
This class wraps chunk of memory into the DAVA::File interface:
- does not allocate memory by itself (don't have internal storage);
- does not release provided memory;
- serves only for reading purposes (used by classes which consumes DAVA::File interface);
- the only one allowed operation is sequential reading.
*/
class UnmanagedMemoryFile : public File
{
public:
    UnmanagedMemoryFile() = default;
    UnmanagedMemoryFile(const uint8* p, uint32 sz);

    uint32 Read(void* destinationBuffer, uint32 dataSize) override;
    bool Seek(int64 position, eFileSeek seekType) override;

    uint64 GetPos() const override;
    uint64 GetSize() const override;
    bool IsEof() const override;

private:
    uint32 Write(const void* sourceBuffer, uint32 dataSize) override;
    bool Flush() override;

private:
    const uint8* pointer = nullptr;
    uint64 size = 0;
    uint64 offset = 0;
};

inline UnmanagedMemoryFile::UnmanagedMemoryFile(const uint8* p, uint32 sz)
    : pointer(p)
    , size(sz)
{
}

inline uint32 UnmanagedMemoryFile::Read(void* destinationBuffer, uint32 dataSize)
{
    DVASSERT(pointer != nullptr);
    DVASSERT(destinationBuffer != nullptr);

    if (offset + dataSize > size)
    {
        dataSize = static_cast<uint32>(size - offset);
    }

    Memcpy(destinationBuffer, pointer + offset, dataSize);
    offset += dataSize;

    return dataSize;
}

inline uint64 UnmanagedMemoryFile::GetPos() const
{
    return offset;
}

inline uint64 UnmanagedMemoryFile::GetSize() const
{
    return size;
}

inline bool UnmanagedMemoryFile::IsEof() const
{
    return offset == size;
}

inline bool UnmanagedMemoryFile::Seek(int64 position, eFileSeek seekType)
{
    switch (seekType)
    {
    case eFileSeek::SEEK_FROM_START:
    {
        if ((position >= 0) && (position < static_cast<int64>(size)))
        {
            offset = static_cast<uint64>(position);
            return true;
        }
        break;
    }
    case eFileSeek::SEEK_FROM_END:
    {
        if ((position >= 0) && (position < static_cast<int64>(size)))
        {
            offset = size - static_cast<uint64>(position) - 1;
            return true;
        }
        break;
    }
    case eFileSeek::SEEK_FROM_CURRENT:
    {
        position += static_cast<int64>(offset);
        if ((position >= 0) && (position < static_cast<int64>(size)))
        {
            offset = static_cast<uint64>(position);
            return true;
        }
        break;
    }
    default:
        DVASSERT(!"Invalid eFileSeek specified");
    }

    return false;
}

inline uint32 UnmanagedMemoryFile::Write(const void* sourceBuffer, uint32 dataSize)
{
    DVASSERT(0, "Write is not supported");
    return 0;
}

inline bool UnmanagedMemoryFile::Flush()
{
    DVASSERT(0, "Flush is not supported");
    return 0;
}
}
