#include "Render/Shader.h"
#include "Render/RHI/rhi_ShaderCache.h"
#include "Render/RenderBase.h"
#include "Logger/Logger.h"

namespace DAVA
{
struct BufferPropertyLayout
{
    rhi::ShaderPropList props;
};

bool operator==(const BufferPropertyLayout& lhs, const BufferPropertyLayout& rhs)
{
    if (lhs.props.size() != rhs.props.size())
        return false;
    for (size_t i = 0, sz = lhs.props.size(); i < sz; ++i)
    {
        if ((lhs.props[i].uid != rhs.props[i].uid) ||
            (lhs.props[i].type != rhs.props[i].type) ||
            (lhs.props[i].bufferReg != rhs.props[i].bufferReg) ||
            (lhs.props[i].defaultValue != rhs.props[i].defaultValue) || //should we really compare defaultValue HERE!?!?!?!?!?
            (lhs.props[i].bufferRegCount != rhs.props[i].bufferRegCount))
        {
            return false;
        }
    }

    return true;
}

namespace
{
UniqueStateSet<BufferPropertyLayout> propertyLayoutSet;
}

uint32 ShaderDescriptor::CalculateRegsCount(rhi::ShaderProp::Type type, uint32 arraySize)
{
    switch (type)
    {
    case rhi::ShaderProp::TYPE_FLOAT1:
    case rhi::ShaderProp::TYPE_FLOAT2:
    case rhi::ShaderProp::TYPE_FLOAT3:
        DVASSERT(arraySize == 1); //arrays of non register aligned types are not supported
        return 1;
    case rhi::ShaderProp::TYPE_FLOAT4:
        return arraySize; //1 float4 register per array element
    case rhi::ShaderProp::TYPE_FLOAT4X4:
        return arraySize * 4; //4 float4 register per array element
    default:
        DVASSERT(false); //how did we get here? unknown property type
        return 0;
    }
}

uint32 ShaderDescriptor::CalculateDataSize(rhi::ShaderProp::Type type, uint32 arraySize)
{
    switch (type)
    {
    case rhi::ShaderProp::TYPE_FLOAT1:
        DVASSERT(arraySize == 1); //arrays of non register aligned types are not supported
        return 1;
    case rhi::ShaderProp::TYPE_FLOAT2:
        DVASSERT(arraySize == 1); //arrays of non register aligned types are not supported
        return 2;
    case rhi::ShaderProp::TYPE_FLOAT3:
        DVASSERT(arraySize == 1); //arrays of non register aligned types are not supported
        return 3;
    case rhi::ShaderProp::TYPE_FLOAT4:
        return arraySize * 4; //1 float4 register per array element
    case rhi::ShaderProp::TYPE_FLOAT4X4:
        return arraySize * 16; //4 float4 register per array element
    default:
        DVASSERT(false); //how did we get here? unknown property type
        return 0;
    }
}

const rhi::ShaderPropList& ShaderDescriptor::GetProps(UniquePropertyLayout layout)
{
    return propertyLayoutSet.GetUnique(layout).props;
}

void ShaderDescriptor::UpdateDynamicParams()
{
    //Logger::Info( " upd-dyn-params" );
    for (auto& dynamicBinding : dynamicPropertyBindings)
    {
        if (dynamicBinding.buffer == rhi::InvalidHandle) //buffer is cut by compiler/linker!
            continue;
        const float32* data = static_cast<const float32*>(Renderer::GetDynamicBindings().GetDynamicParam(dynamicBinding.dynamicPropertySemantic));
        pointer_size updateSemantic = Renderer::GetDynamicBindings().GetDynamicParamUpdateSemantic(dynamicBinding.dynamicPropertySemantic);
        if (dynamicBinding.updateSemantic != updateSemantic)
        {
            if (dynamicBinding.type < rhi::ShaderProp::TYPE_FLOAT4)
            {
                DVASSERT(Renderer::GetDynamicBindings().GetDynamicParamArraySize(dynamicBinding.dynamicPropertySemantic) == 1);
                rhi::UpdateConstBuffer1fv(dynamicBinding.buffer, dynamicBinding.reg, dynamicBinding.regCount, data, CalculateDataSize(dynamicBinding.type, 1));
            }
            else
            {
                uint32 arraySize = Renderer::GetDynamicBindings().GetDynamicParamArraySize(dynamicBinding.dynamicPropertySemantic, dynamicBinding.arraySize);
                DVASSERT(arraySize <= dynamicBinding.regCount);
                rhi::UpdateConstBuffer4fv(dynamicBinding.buffer, dynamicBinding.reg, data, CalculateRegsCount(dynamicBinding.type, arraySize));
            }

            dynamicBinding.updateSemantic = updateSemantic;

#if defined(__DAVAENGINE_RENDERSTATS__)
            ++Renderer::GetRenderStats().dynamicParamBindCount;
#endif
        }
    }
}

void ShaderDescriptor::ClearDynamicBindings()
{
    for (auto& dynamicBinding : dynamicPropertyBindings)
        dynamicBinding.updateSemantic = 0;
}

uint32 ShaderDescriptor::GetVertexConstBuffersCount()
{
    return vertexConstBuffersCount;
}
uint32 ShaderDescriptor::GetFragmentConstBuffersCount()
{
    return fragmentConstBuffersCount;
}

rhi::HConstBuffer ShaderDescriptor::GetDynamicBuffer(ConstBufferDescriptor::Type type, uint32 index)
{
    DVASSERT(dynamicBuffers.find(std::make_pair(type, index)) != dynamicBuffers.end()); //dynamic buffer not found
    return dynamicBuffers[std::make_pair(type, index)];
}

ShaderDescriptor::ShaderDescriptor(rhi::HPipelineState _pipelineState, FastName _vProgUid, FastName _fProgUid)
    : vProgUid(_vProgUid)
    , fProgUid(_fProgUid)
    , piplineState(_pipelineState)
{
}
ShaderDescriptor::~ShaderDescriptor()
{
    rhi::ReleaseRenderPipelineState(piplineState);
}

void ShaderDescriptor::UpdateConfigFromSource(rhi::ShaderSource* vSource, rhi::ShaderSource* fSource)
{
    vertexConstBuffersCount = vSource->ConstBufferCount();
    fragmentConstBuffersCount = fSource->ConstBufferCount();

    constBuffers.clear();
    dynamicBuffers.clear();
    dynamicPropertyBindings.clear();

    Vector<BufferPropertyLayout> bufferPropertyLayouts;
    bufferPropertyLayouts.resize(vertexConstBuffersCount + fragmentConstBuffersCount);

    constBuffers.resize(vertexConstBuffersCount + fragmentConstBuffersCount);

    for (auto& prop : vSource->Properties())
    {
        if (prop.bufferindex >= vertexConstBuffersCount)
        {
            Logger::Error("[UpdateConfigFromSource] Invalid vertex const-buffer index. Index: %u, Count: %u", prop.bufferindex, vertexConstBuffersCount);
            Logger::Error("Shader source code:");
            Logger::Error("%s", vSource->GetSourceCode(rhi::HostApi()).c_str());
        }

        bufferPropertyLayouts[prop.bufferindex].props.push_back(prop);
    }
    for (auto& prop : fSource->Properties())
    {
        if (prop.bufferindex >= fragmentConstBuffersCount)
        {
            Logger::Error("[UpdateConfigFromSource] Invalid fragment const-buffer index. Index: %u, Count: %u", prop.bufferindex, fragmentConstBuffersCount);
            Logger::Error("Shader source code:");
            Logger::Error("%s", fSource->GetSourceCode(rhi::HostApi()).c_str());
        }

        bufferPropertyLayouts[prop.bufferindex + vertexConstBuffersCount].props.push_back(prop);
    }
    for (uint32 i = 0, sz = static_cast<uint32>(constBuffers.size()); i < sz; ++i)
    {
        if (i < vertexConstBuffersCount)
        {
            constBuffers[i].type = ConstBufferDescriptor::Type::Vertex;
            constBuffers[i].targetSlot = i;
            constBuffers[i].updateType = vSource->ConstBufferSource(constBuffers[i].targetSlot);
        }
        else
        {
            constBuffers[i].type = ConstBufferDescriptor::Type::Fragment;
            constBuffers[i].targetSlot = i - vertexConstBuffersCount;
            constBuffers[i].updateType = fSource->ConstBufferSource(constBuffers[i].targetSlot);
        }

        constBuffers[i].propertyLayoutId = propertyLayoutSet.MakeUnique(bufferPropertyLayouts[i]);
    }

    for (size_t i = 0, sz = constBuffers.size(); i < sz; ++i)
    {
        if (constBuffers[i].updateType == rhi::ShaderProp::SOURCE_AUTO)
        {
            rhi::HConstBuffer dynamicBufferHandle;
            if (constBuffers[i].type == ConstBufferDescriptor::Type::Vertex)
                dynamicBufferHandle = rhi::CreateVertexConstBuffer(piplineState, constBuffers[i].targetSlot);
            else
                dynamicBufferHandle = rhi::CreateFragmentConstBuffer(piplineState, constBuffers[i].targetSlot);

            dynamicBuffers[std::make_pair(constBuffers[i].type, constBuffers[i].targetSlot)] = dynamicBufferHandle;
            for (auto& prop : bufferPropertyLayouts[i].props)
            {
                /*for some reason c++11 cant initialize inherited data*/
                DynamicPropertyBinding binding;
                binding.type = prop.type;
                binding.buffer = dynamicBufferHandle;
                binding.reg = prop.bufferReg;
                binding.regCount = prop.bufferRegCount;
                binding.arraySize = prop.arraySize;
                binding.updateSemantic = 0;
                binding.dynamicPropertySemantic = DynamicBindings::GetUniformSemanticByName(prop.uid);
                if (binding.dynamicPropertySemantic == DynamicBindings::UNKNOWN_SEMANTIC)
                    Logger::Error("wrong semantics in prop \"%s\"", prop.uid.c_str());
                DVASSERT(binding.dynamicPropertySemantic != DynamicBindings::UNKNOWN_SEMANTIC); //unknown dynamic property
                dynamicPropertyBindings.push_back(binding);
            }
        }
    }
    vertexSamplerList = vSource->Samplers();
    fragmentSamplerList = fSource->Samplers();
}
}
