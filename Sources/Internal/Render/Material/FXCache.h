#ifndef __DAVAENGINE_FXCACHE_H__
#define __DAVAENGINE_FXCACHE_H__

#include "Render/Shader.h"
#include "Render/RHI/rhi_Type.h"
#include "Render/Highlevel/RenderLayer.h"

namespace DAVA
{
struct RenderPassDescriptor
{
    FastName passName;
    FastName shaderFileName;
    UnorderedMap<FastName, int> templateDefines = UnorderedMap<FastName, int>(8);
    ShaderDescriptor* shader = nullptr;
    bool hasBlend = false;
    rhi::DepthStencilState::Descriptor depthStateDescriptor;
    rhi::HDepthStencilState depthStencilState;
    RenderLayer::eRenderLayerID renderLayer = RenderLayer::RENDER_LAYER_INVALID_ID;
    rhi::CullMode cullMode = rhi::CULL_NONE;
    bool wireframe = false;
};

struct FXDescriptor
{
    Vector<RenderPassDescriptor> renderPassDescriptors;

    //for storing and further debug simplification
    FastName fxName;
    UnorderedMap<FastName, int32> defines = UnorderedMap<FastName, int32>(16);
};

namespace FXCache
{
void Initialize();
void Uninitialize();
void Clear();
const FXDescriptor& GetFXDescriptor(const FastName& fxName, UnorderedMap<FastName, int32>& defines, const FastName& quality = NMaterialQualityName::DEFAULT_QUALITY_NAME);
}
}


#endif //endif