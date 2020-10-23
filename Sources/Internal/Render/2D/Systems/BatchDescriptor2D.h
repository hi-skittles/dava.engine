#pragma once

#include "Base/BaseTypes.h"
#include "Math/Color.h"
#include "Render/RHI/rhi_Public.h"

namespace DAVA
{
class NMaterial;
struct Matrix4;

struct BatchDescriptor2D
{
    static const uint32 MAX_TEXTURE_STREAMS_COUNT = 4;

    uint32 vertexCount = 0;
    uint32 indexCount = 0;
    uint32 vertexStride = 0;
    uint32 texCoordStride = 0;
    uint32 texCoordCount = 1;
    uint32 colorStride = 0;
    rhi::HTextureSet textureSetHandle;
    rhi::HSamplerState samplerStateHandle;
    rhi::PrimitiveType primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    const float32* vertexPointer = nullptr;
    Array<const float32*, MAX_TEXTURE_STREAMS_COUNT> texCoordPointer = {};
    const uint32* colorPointer = nullptr;
    const uint16* indexPointer = nullptr;
    NMaterial* material = nullptr;
    Matrix4* worldMatrix = nullptr;
    Color singleColor = Color::White;
};
}