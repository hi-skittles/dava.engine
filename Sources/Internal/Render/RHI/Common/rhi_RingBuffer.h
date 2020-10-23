#ifndef __RHI_RINGBUFFER_H__
#define __RHI_RINGBUFFER_H__

#include "rhi_Utils.h"
#include "Logger/Logger.h"
#include "MemoryManager/MemoryProfiler.h"

namespace rhi
{
class
RingBuffer
{
public:
    RingBuffer();

    void Initialize(unsigned sz);
    void Initialize(void* data, unsigned sz);
    void Uninitialize();

    float* Alloc(unsigned cnt, unsigned align = 16);
    void Reset();

private:
    unsigned size;
    uint8* dataPtr;
    uint8* cur;

    unsigned memUsed;
    unsigned allocCount;

    unsigned ownData : 1;
};

//------------------------------------------------------------------------------

inline RingBuffer::RingBuffer()
    : size(0)
    , dataPtr(0)
    , cur(0)
    , memUsed(0)
    , allocCount(0)
    , ownData(false)
{
}

//------------------------------------------------------------------------------

inline void RingBuffer::Initialize(unsigned sz)
{
    DAVA_MEMORY_PROFILER_ALLOC_SCOPE(DAVA::ALLOC_POOL_RHI_BUFFER);

    size = sz;
    dataPtr = static_cast<uint8*>(::malloc(sz));
    ownData = true;

    cur = dataPtr;
    memUsed = 0;
    allocCount = 0;
}

//------------------------------------------------------------------------------

inline void
RingBuffer::Initialize(void* data, unsigned sz)
{
    size = sz;
    dataPtr = static_cast<uint8*>(data);
    ownData = false;

    cur = dataPtr;
    memUsed = 0;
    allocCount = 0;
}

//------------------------------------------------------------------------------

inline void
RingBuffer::Uninitialize()
{
    if (dataPtr && ownData)
        ::free(dataPtr);

    size = 0;
    dataPtr = 0;
    cur = 0;
}

//------------------------------------------------------------------------------

inline float* RingBuffer::Alloc(unsigned cnt, unsigned align)
{
    DVASSERT(cur);

    unsigned sz = L_ALIGNED_SIZE(static_cast<unsigned>(cnt * sizeof(float)), align);
    uint8* buf = cur + sz;
    uint8* p = cur;

    if (buf >= dataPtr + size)
    {
        buf = dataPtr + sz;
        p = dataPtr;
    }

    cur = buf;
    memUsed += sz;
    ++allocCount;

    return reinterpret_cast<float*>(p);
}

//------------------------------------------------------------------------------

inline void RingBuffer::Reset()
{
    if (memUsed > size / 2)
        DAVA::Logger::Warning("const-buffer high-watermark passed (%u of %u used)", memUsed, size);

    /*
static unsigned peak=0;
if( memUsed > peak )
{
peak = memUsed;
DAVA::Logger::Info("ring-buf peak : used %u Kb in %u blocks",memUsed/1024,allocCount);
}
*/
    memUsed = 0;
    allocCount = 0;
}

} // namespace rhi
#endif // __RHI_RINGBUFFER_H__
