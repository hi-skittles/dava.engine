#include "RulerToolProxy.h"

RulerToolProxy::RulerToolProxy(DAVA::int32 size)
    : size(size)
    , spriteChanged(false)
{
    DAVA::uint32 unsignedSize = static_cast<DAVA::uint32>(size);
    rulerToolTexture = DAVA::Texture::CreateFBO(unsignedSize, unsignedSize, DAVA::FORMAT_RGBA8888);

    rhi::Viewport viewport;
    viewport.x = viewport.y = 0U;
    viewport.width = unsignedSize;
    viewport.height = unsignedSize;
    DAVA::RenderHelper::CreateClearPass(rulerToolTexture->handle, rhi::HTexture(), DAVA::PRIORITY_CLEAR, DAVA::Color(0.f, 0.f, 0.f, 0.f), viewport);
}

RulerToolProxy::~RulerToolProxy()
{
    SafeRelease(rulerToolTexture);
}

DAVA::int32 RulerToolProxy::GetSize()
{
    return size;
}

DAVA::Texture* RulerToolProxy::GetTexture()
{
    return rulerToolTexture;
}
