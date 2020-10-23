#if !defined __RHI_RINGBUFFERMETAL_H__
#define __RHI_RINGBUFFERMETAL_H__

    #include "../Common/rhi_RingBuffer.h"
    #include "_metal.h"
#if !(TARGET_IPHONE_SIMULATOR == 1)
namespace rhi
{
class
RingBufferMetal
{
public:
    void Initialize(unsigned sz);
    void Uninitialize();

    float* Alloc(unsigned cnt, unsigned* offset = 0);
    void Reset();

    id<MTLBuffer> BufferUID() const;
    unsigned Offset(void* ptr) const;

private:
    RingBuffer buf;
    __unsafe_unretained id<MTLBuffer> uid;
};

} // namespace rhi
#endif //#if !(TARGET_IPHONE_SIMULATOR==1)

#endif // __RHI_RINGBUFFERMETAL_H__
