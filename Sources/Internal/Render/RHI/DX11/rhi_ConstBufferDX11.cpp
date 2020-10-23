#include "rhi_DX11.h"
#include "../rhi_ShaderCache.h"
#include <D3D11Shader.h>
#include <D3Dcompiler.h>

namespace rhi
{
class ConstBufDX11_t
{
public:
    static RingBuffer defaultRingBuffer;
    static uint32 currentFrame;

    struct Desc
    {
    };

public:
    void Construct(ProgType type, uint32 buf_i, uint32 reg_count);
    void Destroy();

    uint32 ConstCount();

    bool SetConst(uint32 const_i, uint32 count, const float* data);
    bool SetConst(uint32 const_i, uint32 const_sub_i, const float* data, uint32 dataCount);
    void SetToRHI(ID3D11DeviceContext* context, ID3D11Buffer** buffer);
    void Invalidate();
    void SetToRHI(const void* instData);

    const void* Instance();

private:
    ProgType progType = PROG_VERTEX;
    ID3D11Buffer* buffer = nullptr;
    float* value = nullptr;
    float* inst = nullptr;
    uint32 frame = 0;
    uint32 buf_i = DAVA::InvalidIndex;
    uint32 regCount = 0;
    uint32 lastUpdateFrame = static_cast<uint32>(-1);
};
using ConstBufDX11Pool = ResourcePool<ConstBufDX11_t, RESOURCE_CONST_BUFFER, ConstBufDX11_t::Desc, false>;
RHI_IMPL_POOL_SIZE(ConstBufDX11_t, RESOURCE_CONST_BUFFER, ConstBufDX11_t::Desc, false, 12 * 1024);

RingBuffer ConstBufDX11_t::defaultRingBuffer;
uint32 ConstBufDX11_t::currentFrame = 0;

void ConstBufDX11_t::Construct(ProgType ptype, uint32 bufIndex, uint32 regCnt)
{
    DVASSERT(value == nullptr);
    DVASSERT(bufIndex != DAVA::InvalidIndex);
    DVASSERT(regCnt);

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = regCnt * (4 * sizeof(float));
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (DX11DeviceCommand(DX11Command::CREATE_BUFFER, &desc, NULL, &buffer))
    {
        value = reinterpret_cast<float*>(calloc(regCnt, (4 * sizeof(float))));
        progType = ptype;
        buf_i = bufIndex;
        regCount = regCnt;
        lastUpdateFrame = currentFrame;
    }
}

void ConstBufDX11_t::Destroy()
{
    DAVA::SafeRelease(buffer);
    if (value)
        ::free(value);

    frame = 0;
    regCount = 0;
    inst = nullptr;
    value = nullptr;
    lastUpdateFrame = static_cast<uint32>(0);
    buf_i = DAVA::InvalidIndex;
}

uint32 ConstBufDX11_t::ConstCount()
{
    return regCount;
}

bool ConstBufDX11_t::SetConst(uint32 const_i, uint32 const_count, const float* data)
{
    DVASSERT(const_i + const_count <= regCount);

    memcpy(value + 4 * const_i, data, const_count * (4 * sizeof(float)));

    if (dx11.useHardwareCommandBuffers)
    {
        lastUpdateFrame = currentFrame;
    }
    else
    {
        inst = nullptr;
    }
    return true;
}

bool ConstBufDX11_t::SetConst(uint32 const_i, uint32 const_sub_i, const float* data, uint32 dataCount)
{
    DVASSERT(const_i <= regCount && const_sub_i < 4);

    memcpy(value + const_i * 4 + const_sub_i, data, dataCount * sizeof(float));
    if (dx11.useHardwareCommandBuffers)
    {
        lastUpdateFrame = currentFrame;
    }
    else
    {
        inst = nullptr;
    }

    return true;
}

void ConstBufDX11_t::SetToRHI(ID3D11DeviceContext* context, ID3D11Buffer** outBuffer)
{
    if (lastUpdateFrame == currentFrame)
    {
        DVASSERT(dx11.useHardwareCommandBuffers);
        context->UpdateSubresource(buffer, 0, nullptr, value, regCount * (4 * sizeof(float)), 0);
    }
    outBuffer[buf_i] = buffer;
}

void ConstBufDX11_t::SetToRHI(const void* instData)
{
    dx11.context->UpdateSubresource(buffer, 0, nullptr, instData, regCount * (4 * sizeof(float)), 0);

    if (progType == PROG_VERTEX)
        dx11.context->VSSetConstantBuffers(buf_i, 1, &buffer);
    else
        dx11.context->PSSetConstantBuffers(buf_i, 1, &buffer);
}

const void* ConstBufDX11_t::Instance()
{
    if ((inst == nullptr) || (frame != currentFrame))
    {
        inst = defaultRingBuffer.Alloc(regCount * (4 * sizeof(float)));
        memcpy(inst, value, regCount * (4 * sizeof(float)));
        frame = currentFrame;
    }
    return inst;
}

void ConstBufDX11_t::Invalidate()
{
    lastUpdateFrame = currentFrame;
}

static bool dx11_ConstBuffer_SetConst(Handle cb, uint32 const_i, uint32 const_count, const float* data)
{
    ConstBufDX11_t* cb11 = ConstBufDX11Pool::Get(cb);
    return cb11->SetConst(const_i, const_count, data);
}

static bool dx11_ConstBuffer_SetConst1fv(Handle cb, uint32 const_i, uint32 const_sub_i, const float* data, uint32 dataCount)
{
    ConstBufDX11_t* cb11 = ConstBufDX11Pool::Get(cb);

    return cb11->SetConst(const_i, const_sub_i, data, dataCount);
}

void dx11_ConstBuffer_Delete(Handle cb)
{
    ConstBufDX11_t* cb11 = ConstBufDX11Pool::Get(cb);
    cb11->Destroy();
    ConstBufDX11Pool::Free(cb);
}

Handle ConstBufferDX11::Alloc(ProgType ptype, uint32 bufIndex, uint32 regCnt)
{
    Handle handle = ConstBufDX11Pool::Alloc();
    ConstBufDX11_t* cb = ConstBufDX11Pool::Get(handle);
    cb->Construct(ptype, bufIndex, regCnt);
    return handle;
}

void ConstBufferDX11::Init(uint32 maxCount)
{
    ConstBufDX11Pool::Reserve(maxCount);
}

void ConstBufferDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_ConstBuffer_SetConst = &dx11_ConstBuffer_SetConst;
    dispatch->impl_ConstBuffer_SetConst1fv = &dx11_ConstBuffer_SetConst1fv;
    dispatch->impl_ConstBuffer_Delete = &dx11_ConstBuffer_Delete;
}

void ConstBufferDX11::InvalidateAll()
{
    for (ConstBufDX11Pool::Iterator cb = ConstBufDX11Pool::Begin(), cb_end = ConstBufDX11Pool::End(); cb != cb_end; ++cb)
    {
        cb->Invalidate();
    }
}

void ConstBufferDX11::SetToRHI(Handle cb, ID3D11DeviceContext* context, ID3D11Buffer** buffer)
{
    ConstBufDX11_t* cb11 = ConstBufDX11Pool::Get(cb);
    cb11->SetToRHI(context, buffer);
}

void ConstBufferDX11::SetToRHI(Handle cb, const void* instData)
{
    ConstBufDX11_t* cb11 = ConstBufDX11Pool::Get(cb);
    cb11->SetToRHI(instData);
}

const void* ConstBufferDX11::Instance(Handle cb)
{
    ConstBufDX11_t* cb11 = ConstBufDX11Pool::Get(cb);
    return cb11->Instance();
}

void ConstBufferDX11::InvalidateAllInstances()
{
    ++ConstBufDX11_t::currentFrame;
}

void ConstBufferDX11::InitializeRingBuffer(uint32 size)
{
    DVASSERT(size > 0);

    if (!dx11.useHardwareCommandBuffers)
        ConstBufDX11_t::defaultRingBuffer.Initialize(size);
}

} // namespace rhi
