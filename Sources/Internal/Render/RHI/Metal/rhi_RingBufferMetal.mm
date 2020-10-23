#include "rhi_RingBufferMetal.h"

    #include "_metal.h"
#if !(TARGET_IPHONE_SIMULATOR == 1)
namespace rhi
{
//------------------------------------------------------------------------------

void RingBufferMetal::Initialize(unsigned sz)
{
    uid = [_Metal_Device newBufferWithLength:sz options:MTLResourceOptionCPUCacheModeDefault];
    //    uid = [_Metal_Device newBufferWithLength:sz options:MTLCPUCacheModeWriteCombined];

    buf.Initialize(uid.contents, uid.length);
}

//------------------------------------------------------------------------------

void RingBufferMetal::Uninitialize()
{
}

//------------------------------------------------------------------------------

float*
RingBufferMetal::Alloc(unsigned cnt, unsigned* offset)
{
    float* ptr = buf.Alloc(cnt, 256); // since MTL-buf offset must be aligned to 256
    unsigned off = (uint8*)ptr - (uint8*)(uid.contents);

    if (offset)
        *offset = off;

    return ptr;
}

//------------------------------------------------------------------------------

void
RingBufferMetal::Reset()
{
    buf.Reset();
}

//------------------------------------------------------------------------------

id<MTLBuffer>
RingBufferMetal::BufferUID() const
{
    return uid;
}

//------------------------------------------------------------------------------

unsigned
RingBufferMetal::Offset(void* ptr) const
{
    return (uint8*)ptr - (uint8*)(uid.contents);
}

} // namespace rhi

#endif //#if !(TARGET_IPHONE_SIMULATOR==1)