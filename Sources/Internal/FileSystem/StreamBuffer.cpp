#include "StreamBuffer.h"

#include "Debug/DVAssert.h"
#include "Concurrency/LockGuard.h"

namespace DAVA
{
StreamBuffer::~StreamBuffer()
{
    Clear();
}

void StreamBuffer::Clear()
{
    LockGuard<Mutex> lock(interactionsLock);
    pages.clear();
}

void StreamBuffer::Write(uint8* dataIn, uint32 len)
{
    DVASSERT(nullptr != dataIn);
    LockGuard<Mutex> lock(interactionsLock);
    WriteInternal(dataIn, len);
}

uint32 StreamBuffer::Read(uint8* dataOut, uint32 len)
{
    DVASSERT(nullptr != dataOut);
    LockGuard<Mutex> lock(interactionsLock);
    uint32 bytesRead = 0;
    uint32 readMore = len;
    while (pages.size() > 0 && readMore > 0)
    {
        uint32 readSize = ReadInternal(dataOut + bytesRead, readMore);
        readMore -= readSize;
        bytesRead += readSize;
    }

    size -= bytesRead;
    return bytesRead;
}

uint32 StreamBuffer::GetSize()
{
    return size;
}

void StreamBuffer::WriteInternal(uint8* dataIn, uint32 len)
{
    pages.emplace_back(len);

    Memcpy(pages.back().data(), dataIn, len);

    size += len;
}

uint32 StreamBuffer::ReadInternal(uint8* dataOut, uint32 len)
{
    const uint32 pageSize = static_cast<uint32>(pages.front().size());
    const uint32 dataSizeInPage = pageSize - currentPageReadPos;
    const uint32 sizeToRead = Min(dataSizeInPage, len);
    Memcpy(dataOut, pages.front().data() + currentPageReadPos, sizeToRead);

    currentPageReadPos += sizeToRead;

    if (pageSize == currentPageReadPos)
    {
        pages.pop_front();
        currentPageReadPos = 0;
    }

    return sizeToRead;
}
}
