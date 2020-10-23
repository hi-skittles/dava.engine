#ifndef __DAVAENGINE_SHADER_H__
#define __DAVAENGINE_SHADER_H__

#include "Base/BaseTypes.h"
#include "Render/Renderer.h"
#include "Render/RHI/rhi_ShaderSource.h"
#include "Render/UniqueStateSet.h"
#include "Render/DynamicBindings.h"

namespace DAVA
{
using UniquePropertyLayout = UniqueHandle;

struct ConstBufferDescriptor
{
    enum class Type
    {
        Vertex,
        Fragment
    };

    Type type;
    rhi::ShaderProp::Source updateType;
    uint32 targetSlot;

    UniquePropertyLayout propertyLayoutId;
};

struct DynamicPropertyBinding
{
    rhi::ShaderProp::Type type;
    uint32 reg;
    uint32 regCount; //offset for props less than 1 reg size
    uint32 arraySize;
    pointer_size updateSemantic;
    rhi::HConstBuffer buffer;
    DynamicBindings::eUniformSemantic dynamicPropertySemantic;
};

//forward declarations for friending
class ShaderDescriptor;
namespace ShaderDescriptorCache
{
ShaderDescriptor* GetShaderDescriptor(const FastName& name, const UnorderedMap<FastName, int32>& defines);
void ReloadShaders();
}

class ShaderDescriptor
{
public: //utility
    static const rhi::ShaderPropList& GetProps(UniquePropertyLayout layout);
    static uint32 CalculateRegsCount(rhi::ShaderProp::Type type, uint32 arraySize); //return in registers
    static uint32 CalculateDataSize(rhi::ShaderProp::Type type, uint32 arraySize); //return in float

public:
    void UpdateDynamicParams();
    void ClearDynamicBindings();

    uint32 GetVertexConstBuffersCount();
    uint32 GetFragmentConstBuffersCount();

    rhi::HConstBuffer GetDynamicBuffer(ConstBufferDescriptor::Type type, uint32 index);
    inline rhi::HPipelineState GetPiplineState()
    {
        return piplineState;
    }

    uint32 GetRequiredVertexFormat()
    {
        return requiredVertexFormat;
    }

    const Vector<ConstBufferDescriptor>& GetConstBufferDescriptors() const
    {
        return constBuffers;
    }
    const rhi::ShaderSamplerList& GetFragmentSamplerList() const
    {
        return fragmentSamplerList;
    }
    const rhi::ShaderSamplerList& GetVertexSamplerList() const
    {
        return vertexSamplerList;
    }

    bool IsValid();

private:
    ShaderDescriptor(rhi::HPipelineState pipelineState, FastName vProgUid, FastName fProgUid);
    ~ShaderDescriptor();

    void UpdateConfigFromSource(rhi::ShaderSource* vSource, rhi::ShaderSource* fSource);

    Vector<ConstBufferDescriptor> constBuffers;

    uint32 vertexConstBuffersCount, fragmentConstBuffersCount;
    Vector<DynamicPropertyBinding> dynamicPropertyBindings;

    Map<std::pair<ConstBufferDescriptor::Type, uint32>, rhi::HConstBuffer> dynamicBuffers;

    FastName vProgUid, fProgUid;
    rhi::HPipelineState piplineState;

    uint32 requiredVertexFormat;

    rhi::ShaderSamplerList fragmentSamplerList;
    rhi::ShaderSamplerList vertexSamplerList;

    bool valid;

    //for storing and further debug simplification
    FastName sourceName;
    UnorderedMap<FastName, int32> defines;

    friend ShaderDescriptor* ShaderDescriptorCache::GetShaderDescriptor(const FastName& name, const UnorderedMap<FastName, int32>& defines);
    friend void ShaderDescriptorCache::ReloadShaders();
};

inline bool ShaderDescriptor::IsValid()
{
    return valid;
}
};

#endif // __DAVAENGINE_SHADER_H__
