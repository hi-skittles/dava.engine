#pragma once

#include "rhi_DX11.h"

namespace rhi
{
class BufferDX11_t
{
public:
    ID3D11Buffer* buffer = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

public:
    bool Create(UINT inSize, Usage inUsage, DXGI_FORMAT inFormat, UINT inBindFlags, const void* inInitialData);
    void Destroy();
    bool Update(const void* data, uint32 offset, uint32 size);
    void* Map(uint32 offset, uint32 size);
    void Unmap();
    void ResolvePendingUpdate(ID3D11DeviceContext* context);

private:
    uint32 bufferSize = 0;
    Usage usage = USAGE_STATICDRAW;
    void* mappedData = nullptr;
    bool updatePending = false;
    bool isMapped = false;
};
}