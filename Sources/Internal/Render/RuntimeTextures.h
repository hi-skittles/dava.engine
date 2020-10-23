#ifndef __DAVAENGINE_RUNTIME_TEXTURES_H__
#define __DAVAENGINE_RUNTIME_TEXTURES_H__

#include "Math/Color.h"
#include "Math/Matrix4.h"
#include "Math/Vector.h"
#include "Base/FastName.h"
#include "Render/RHI/rhi_Public.h"
#include "Render/Texture.h"

namespace DAVA
{
class RuntimeTextures
{
public:
    const static int32 REFLECTION_TEX_SIZE = 512;
    const static int32 REFRACTION_TEX_SIZE = 512;

    enum eDynamicTextureSemantic
    {
        TEXTURE_STATIC = 0,
        TEXTURE_DYNAMIC_REFLECTION,
        TEXTURE_DYNAMIC_REFRACTION,
        TEXTURE_DYNAMIC_RR_DEPTHBUFFER, //depth buffer for reflection and refraction
        //later add here shadow maps, environment probes etc.

        DYNAMIC_TEXTURES_END,
        DYNAMIC_TEXTURES_COUNT = DYNAMIC_TEXTURES_END,
    };

    RuntimeTextures()
    {
        pinkTexture[0] = pinkTexture[1] = nullptr;
    }

public:
    static RuntimeTextures::eDynamicTextureSemantic GetDynamicTextureSemanticByName(const FastName& name);

    rhi::HTexture GetDynamicTexture(eDynamicTextureSemantic semantic);
    rhi::SamplerState::Descriptor::Sampler GetDynamicTextureSamplerState(eDynamicTextureSemantic semantic);
    PixelFormat GetDynamicTextureFormat(eDynamicTextureSemantic semantic);

    rhi::HTexture GetPinkTexture(rhi::TextureType type);
    rhi::SamplerState::Descriptor::Sampler GetPinkTextureSamplerState(rhi::TextureType type);

    void ClearRuntimeTextures();

private:
    void InitDynamicTexture(eDynamicTextureSemantic semantic);

    rhi::HTexture dynamicTextures[DYNAMIC_TEXTURES_COUNT];
    PixelFormat dynamicTexturesFormat[DYNAMIC_TEXTURES_COUNT];
    Texture* pinkTexture[2]; //TEXTURE_2D & TEXTURE_CUBE
};
}
#endif // __DAVAENGINE_RUNTIME_TEXTURES_H__
