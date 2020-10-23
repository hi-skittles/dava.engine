#ifndef DAVAENGINE_STREAM_BUFFER_H
#define DAVAENGINE_STREAM_BUFFER_H

#include "Base/BaseTypes.h"
#include "Concurrency/Mutex.h"

namespace DAVA
{
/*
    That class is Thread Safe. Current usage wants it. But in the future it could make overhead, so refactor it when needed.
*/

class StreamBuffer
{
public:
    ~StreamBuffer();
    void Clear();
    void Write(uint8* dataIn, uint32 len);
    uint32 Read(uint8* dataOut, uint32 len);
    uint32 GetSize();

private:
    void WriteInternal(uint8* dataIn, uint32 len);
    uint32 ReadInternal(uint8* dataOut, uint32 len);

private:
    Mutex interactionsLock;
    List<Vector<uint8>> pages;
    uint32 currentPageReadPos = 0;
    uint32 size = 0;
};
}

#endif // !DAVAENGINE_STREAM_BUFFER_H
