#include "Concurrency/LockGuard.h"
#include "rhi_BufferDX11.h"

namespace rhi
{
bool BufferDX11_t::Create(UINT inSize, Usage inUsage, DXGI_FORMAT inFormat, UINT inBindFlags, const void* inInitialData)
{
    DVASSERT(inSize > 0);

    if (inUsage == USAGE_STATICDRAW)
    {
        DVASSERT(inInitialData != nullptr);
    }

    if (inSize == 0)
        return false;

    bufferSize = inSize;
    usage = inUsage;
    format = inFormat;
    updatePending = false;
    isMapped = false;

    D3D11_BUFFER_DESC desc11 = {};
    desc11.ByteWidth = bufferSize;
    desc11.Usage = (usage == USAGE_STATICDRAW) ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DYNAMIC;
    desc11.CPUAccessFlags = (usage == USAGE_STATICDRAW) ? 0 : D3D11_CPU_ACCESS_WRITE;
    desc11.BindFlags = inBindFlags;

    D3D11_SUBRESOURCE_DATA data = {};
    if (inInitialData)
    {
        data.pSysMem = inInitialData;
        data.SysMemPitch = inSize;
    }

    bool commandExecuted = DX11DeviceCommand(DX11Command::CREATE_BUFFER, &desc11, (inInitialData ? &data : nullptr), &buffer);
    return commandExecuted && (buffer != nullptr);
}

void BufferDX11_t::Destroy()
{
    DAVA::SafeRelease(buffer);
    if (mappedData)
        ::free(mappedData);

    bufferSize = 0;
    buffer = nullptr;
    mappedData = nullptr;
    updatePending = false;
}

bool BufferDX11_t::Update(const void* inData, uint32 inOffset, uint32 inSize)
{
    DVASSERT(usage == USAGE_DEFAULT);
    DVASSERT(inOffset + inSize <= bufferSize);
    DVASSERT(isMapped == false);

    D3D11_MAPPED_SUBRESOURCE rc = {};
    DX11Command cmd1(DX11Command::MAP, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &rc);
    ExecDX11(&cmd1, 1);
    bool success = DX11Check(cmd1.retval) && (rc.pData != nullptr);
    if (success)
    {
        memcpy(reinterpret_cast<uint8*>(rc.pData) + inOffset, inData, inSize);
        DX11Command cmd2(DX11Command::UNMAP, buffer, 0);
        ExecDX11(&cmd2, 1);
    }
    return success;
}

void* BufferDX11_t::Map(uint32 inOffset, uint32 inSize)
{
    DVASSERT(inOffset + inSize <= bufferSize);
    DVASSERT(usage != USAGE_STATICDRAW);
    DVASSERT(isMapped == false);

    void* ptr = nullptr;
    if (usage == USAGE_DYNAMICDRAW)
    {
        if (dx11.useHardwareCommandBuffers)
        {
            DAVA::LockGuard<DAVA::Mutex> lock(dx11.deferredContextLock);

            D3D11_MAPPED_SUBRESOURCE rc = {};
            HRESULT hr = dx11.deferredContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &rc);
            if (DX11Check(hr) && (rc.pData != nullptr))
            {
                isMapped = true;
                ptr = rc.pData;
            }
        }
        else
        {
            if (mappedData == nullptr)
                mappedData = ::malloc(bufferSize);

            isMapped = true;
            ptr = mappedData;
        }
    }
    else
    {
        D3D11_MAPPED_SUBRESOURCE rc = {};
        DX11Command cmd(DX11Command::MAP, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &rc);
        ExecDX11(&cmd, 1);
        if (DX11Check(cmd.retval) && (rc.pData != nullptr))
        {
            DVASSERT(inOffset + inSize <= rc.RowPitch);
            DVASSERT(bufferSize <= rc.RowPitch);
            ptr = rc.pData;
            isMapped = true;
        }
    }

    return reinterpret_cast<uint8*>(ptr) + inOffset;
}

void BufferDX11_t::Unmap()
{
    DVASSERT(usage != USAGE_STATICDRAW);
    DVASSERT(isMapped);

    if (usage == USAGE_DYNAMICDRAW)
    {
        if (dx11.useHardwareCommandBuffers)
        {
            DAVA::LockGuard<DAVA::Mutex> lock(dx11.deferredContextLock);
            dx11.deferredContext->Unmap(buffer, 0);
        }
        else
        {
            updatePending = true;
        }
    }
    else
    {
        DX11Command cmd(DX11Command::UNMAP, buffer, 0);
        ExecDX11(&cmd, 1);
    }
    isMapped = false;
}

void BufferDX11_t::ResolvePendingUpdate(ID3D11DeviceContext* context)
{
    if (updatePending == false)
        return;

    DVASSERT(dx11.useHardwareCommandBuffers == false);

    D3D11_MAPPED_SUBRESOURCE rc = {};
    HRESULT hr = context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &rc);
    if (DX11Check(hr) && rc.pData)
    {
        DVASSERT(bufferSize <= rc.RowPitch);
        memcpy(rc.pData, mappedData, bufferSize);
        context->Unmap(buffer, 0);
    }
    updatePending = false;
}
}