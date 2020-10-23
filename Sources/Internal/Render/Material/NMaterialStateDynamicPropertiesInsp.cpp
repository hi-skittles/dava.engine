#include "Render/Material/NMaterial.h"
#include "Render/Material/FXCache.h"
#include "Render/Material/NMaterialStateDynamicPropertiesInsp.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"

namespace DAVA
{
///////////////////////////////////////////////////////////////////////////
///// NMaterialStateDynamicPropertiesInsp implementation

namespace DefaultValues
{
const Vector3 defaultVec3;
const Color defaultColor(1.0f, 0.0f, 0.0f, 1.0f);
const Color blackColor(0.0f, 0.0f, 0.0f, 1.0f);
const float32 defaultFloat05 = 0.5f;
const float32 defaultFloat10 = 1.0f;
const Vector2 defaultVec2;
const Vector2 defaultVec2I(1.f, 1.f);
const float32 defaultFogStart = 0.0f;
const float32 defaultFogEnd = 500.0f;
const float32 defaultFogHeight = 50.0f;
const float32 defaultFogDensity = 0.005f;
};

void NMaterialStateDynamicPropertiesInsp::FindMaterialPropertiesRecursive(NMaterial* material, PropDataMap& propsMap) const
{
    UnorderedMap<FastName, int32> flags;
    material->CollectMaterialFlags(flags);

    // shader data
    auto fxName = material->GetEffectiveFXName();
    if (fxName.IsValid())
    {
        auto qualityGroup = QualitySettingsSystem::Instance()->GetCurMaterialQuality(material->qualityGroup);
        FXDescriptor fxDescriptor = FXCache::GetFXDescriptor(fxName, flags, qualityGroup);
        for (auto& descriptor : fxDescriptor.renderPassDescriptors)
        {
            if (!descriptor.shader->IsValid())
                continue;

            for (const auto& buff : descriptor.shader->GetConstBufferDescriptors())
            {
                const rhi::ShaderPropList& props = ShaderDescriptor::GetProps(buff.propertyLayoutId);
                for (const auto& prop : props)
                {
                    if (prop.source != rhi::ShaderProp::SOURCE_AUTO)
                    {
                        PropData data;
                        data.size = prop.arraySize;
                        data.type = prop.type;
                        data.defaultValue = prop.defaultValue;
                        propsMap[prop.uid] = data;
                    }
                }
            }
        }
    }
    else
    {
        // if fxName is not valid (e.g global material)
        // we just add all local properties
        const MaterialConfig& config = material->GetCurrentConfig();
        for (const auto& lp : config.localProperties)
        {
            PropData data;
            data.size = lp.second->arraySize;
            data.type = lp.second->type;
            data.defaultValue = nullptr;
            propsMap[lp.first] = data;
        }
    }

    { //TODO: not shader properties need to be added to material
        static const Map<FastName, NMaterialStateDynamicPropertiesInsp::PropData> NOT_SHADER_PROPS =
        {
          { NMaterialParamName::PARAM_LIGHTMAP_SIZE, { 1, rhi::ShaderProp::TYPE_FLOAT1, &NMaterial::DEFAULT_LIGHTMAP_SIZE } },
        };

        // 'insert' with same key will not update value of the element, so do 'insert' in reverse order to allow overrides from code above.
        propsMap.insert(NOT_SHADER_PROPS.rbegin(), NOT_SHADER_PROPS.rend());
    }

    if (material->GetParent())
    {
        FindMaterialPropertiesRecursive(material->GetParent(), propsMap);
    }
}

InspInfoDynamic::DynamicData NMaterialStateDynamicPropertiesInsp::Prepare(void* object, int filter) const
{
    NMaterial* material = static_cast<NMaterial*>(object);
    DVASSERT(material);

    auto data = new PropDataMap();
    FindMaterialPropertiesRecursive(material, *data);

    // user require predefined global properties
    if (filter > 0)
    {
        FillGlobalMaterialMemebers(material, *data);
    }

    DynamicData ret;
    ret.object = object;
    ret.data = std::shared_ptr<void>(data);
    return ret;
}

Vector<FastName> NMaterialStateDynamicPropertiesInsp::MembersList(const DynamicData& ddata) const
{
    Vector<FastName> ret;

    UnorderedMap<FastName, PropData>* members = static_cast<UnorderedMap<FastName, PropData>*>(ddata.data.get());

    DVASSERT(members);

    ret.reserve(members->size());

    auto it = members->cbegin();

    while (it != members->cend())
    {
        ret.push_back(it->first);
        ++it;
    }

    return ret;
}

InspDesc NMaterialStateDynamicPropertiesInsp::MemberDesc(const DynamicData& ddata, const FastName& member) const
{
    return InspDesc(member.c_str());
}

int NMaterialStateDynamicPropertiesInsp::MemberFlags(const DynamicData& ddata, const FastName& member) const
{
    int flags = 0;

    NMaterial* material = static_cast<NMaterial*>(ddata.object);
    DVASSERT(material);

    flags |= I_VIEW;

    if (material->HasLocalProperty(member))
    {
        flags |= I_EDIT;
    }

    return flags;
}

VariantType NMaterialStateDynamicPropertiesInsp::MemberValueGet(const DynamicData& ddata, const FastName& key) const
{
    VariantType ret;

    NMaterial* material = static_cast<NMaterial*>(ddata.object);

    DVASSERT(material);

    UnorderedMap<FastName, PropData>* members = static_cast<UnorderedMap<FastName, PropData>*>(ddata.data.get());

    DVASSERT(members);

    auto it = members->find(key);

    if (it != members->end())
    {
        const PropData& prop = it->second;
        const float32* value = material->GetEffectivePropValue(key);

        if (nullptr == value)
        {
            value = prop.defaultValue;

            if (nullptr == value)
                return ret;
        }

        switch (prop.type)
        {
        case rhi::ShaderProp::TYPE_FLOAT1:
            ret.SetFloat(*value);
            break;

        case rhi::ShaderProp::TYPE_FLOAT2:
            ret.SetVector2(*reinterpret_cast<const Vector2*>(value));
            break;

        case rhi::ShaderProp::TYPE_FLOAT3:
            if (IsColor(key))
            {
                ret.SetColor(Color(value[0], value[1], value[2], 1.0));
            }
            else
            {
                ret.SetVector3(*reinterpret_cast<const Vector3*>(value));
            }
            break;

        case rhi::ShaderProp::TYPE_FLOAT4:
            if (IsColor(key))
            {
                ret.SetColor(Color(value[0], value[1], value[2], value[3]));
            }
            else
            {
                ret.SetVector4(*reinterpret_cast<const Vector4*>(value));
            }
            break;

        case rhi::ShaderProp::TYPE_FLOAT4X4:
            ret.SetMatrix4(*reinterpret_cast<const Matrix4*>(value));
            break;

        default:
            DVASSERT(false);
            break;
        }
    }

    return ret;
}

bool NMaterialStateDynamicPropertiesInsp::IsColor(const FastName& propName) const
{
    return (nullptr != strstr(propName.c_str(), "Color"));
}

void NMaterialStateDynamicPropertiesInsp::MemberValueSet(const DynamicData& ddata, const FastName& key, const VariantType& value)
{
    NMaterial* material = static_cast<NMaterial*>(ddata.object);

    DVASSERT(material);

    UnorderedMap<FastName, PropData>* members = static_cast<UnorderedMap<FastName, PropData>*>(ddata.data.get());

    DVASSERT(members);

    auto it = members->find(key);

    if (it != members->end())
    {
        PropData prop = it->second;

        if (value.GetType() == VariantType::TYPE_NONE /* && state->GetMaterialType() != NMaterial::MATERIALTYPE_GLOBAL */)
        {
            // empty variant value should remove this property
            if (material->HasLocalProperty(key))
            {
                material->RemoveProperty(key);
            }
        }
        else
        {
            auto setValue = [&material, &key, &prop](const float32* value) {
                if (material->HasLocalProperty(key))
                {
                    material->SetPropertyValue(key, value);
                }
                else
                {
                    material->AddProperty(key, value, prop.type, prop.size);
                }
            };

            switch (prop.type)
            {
            case rhi::ShaderProp::TYPE_FLOAT1:
            {
                float32 val = value.AsFloat();
                setValue(&val);
            }
            break;

            case rhi::ShaderProp::TYPE_FLOAT2:
            {
                const Vector2& val = value.AsVector2();
                setValue(val.data);
            }
            break;

            case rhi::ShaderProp::TYPE_FLOAT3:
            {
                if (IsColor(key))
                {
                    const Color& c = value.AsColor();
                    setValue(c.color);
                }
                else
                {
                    const Vector3& val = value.AsVector3();
                    setValue(val.data);
                }
            }
            break;

            case rhi::ShaderProp::TYPE_FLOAT4:
            {
                Vector4 val;

                if (IsColor(key))
                {
                    const Color& c = value.AsColor();
                    setValue(c.color);
                }
                else
                {
                    const Vector4& val = value.AsVector4();
                    setValue(val.data);
                }
            }
            break;

            case rhi::ShaderProp::TYPE_FLOAT4X4:
            {
                const Matrix4& val = value.AsMatrix4();
                setValue(val.data);
            }
            break;

            default:
                DVASSERT(false);
                break;
            }
        }
    }
}

void NMaterialStateDynamicPropertiesInsp::FillGlobalMaterialMemebers(NMaterial* state, PropDataMap& data) const
{
    auto checkAndAdd = [&data](const FastName& name, rhi::ShaderProp::Type type, uint32 size, const float32* defaultValue) {
        auto it = data.find(name);

        if (it == data.end())
        {
            PropData prop;
            prop.type = type;
            prop.defaultValue = defaultValue;
            prop.size = size;

            data[name] = prop;
        }
        else
        {
            if (nullptr == it->second.defaultValue)
            {
                it->second.defaultValue = defaultValue;
            }
        }
    };

    checkAndAdd(NMaterialParamName::PARAM_LIGHT_POSITION0, rhi::ShaderProp::TYPE_FLOAT3, 1, DefaultValues::defaultVec3.data);
    checkAndAdd(NMaterialParamName::PARAM_PROP_AMBIENT_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_PROP_DIFFUSE_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_PROP_SPECULAR_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_LIGHT_AMBIENT_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_LIGHT_DIFFUSE_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_LIGHT_SPECULAR_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_LIGHT_INTENSITY0, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFloat05);
    checkAndAdd(NMaterialParamName::PARAM_MATERIAL_SPECULAR_SHININESS, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFloat05);

    checkAndAdd(NMaterialParamName::PARAM_FOG_LIMIT, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFloat10);
    checkAndAdd(NMaterialParamName::PARAM_FOG_COLOR, rhi::ShaderProp::TYPE_FLOAT3, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_FOG_DENSITY, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFogDensity);
    checkAndAdd(NMaterialParamName::PARAM_FOG_START, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFogStart);
    checkAndAdd(NMaterialParamName::PARAM_FOG_END, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFogEnd);
    checkAndAdd(NMaterialParamName::PARAM_FOG_HALFSPACE_DENSITY, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFogDensity);
    checkAndAdd(NMaterialParamName::PARAM_FOG_HALFSPACE_FALLOFF, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFogDensity);
    checkAndAdd(NMaterialParamName::PARAM_FOG_HALFSPACE_HEIGHT, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFogHeight);
    checkAndAdd(NMaterialParamName::PARAM_FOG_HALFSPACE_LIMIT, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFloat10);
    checkAndAdd(NMaterialParamName::PARAM_FOG_ATMOSPHERE_COLOR_SUN, rhi::ShaderProp::TYPE_FLOAT3, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_FOG_ATMOSPHERE_COLOR_SKY, rhi::ShaderProp::TYPE_FLOAT3, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_FOG_ATMOSPHERE_SCATTERING, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFloat10);
    checkAndAdd(NMaterialParamName::PARAM_FOG_ATMOSPHERE_DISTANCE, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFogEnd);

    checkAndAdd(NMaterialParamName::PARAM_LIGHTMAP_SIZE, rhi::ShaderProp::TYPE_FLOAT1, 1, &NMaterial::DEFAULT_LIGHTMAP_SIZE);
    checkAndAdd(NMaterialParamName::PARAM_FLAT_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::PARAM_TEXTURE0_SHIFT, rhi::ShaderProp::TYPE_FLOAT2, 1, DefaultValues::defaultVec2.data);
    checkAndAdd(NMaterialParamName::PARAM_UV_OFFSET, rhi::ShaderProp::TYPE_FLOAT2, 1, DefaultValues::defaultVec2.data);
    checkAndAdd(NMaterialParamName::PARAM_UV_SCALE, rhi::ShaderProp::TYPE_FLOAT2, 1, DefaultValues::defaultVec2.data);
    checkAndAdd(NMaterialParamName::PARAM_DECAL_TILE_SCALE, rhi::ShaderProp::TYPE_FLOAT2, 1, DefaultValues::defaultVec2.data);
    checkAndAdd(NMaterialParamName::PARAM_DECAL_TILE_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, Color::White.color);
    checkAndAdd(NMaterialParamName::PARAM_DETAIL_TILE_SCALE, rhi::ShaderProp::TYPE_FLOAT2, 1, DefaultValues::defaultVec2.data);
    checkAndAdd(NMaterialParamName::DEPRECATED_SHADOW_COLOR_PARAM, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::defaultColor.color);
    checkAndAdd(NMaterialParamName::FORCED_SHADOW_DIRECTION_PARAM, rhi::ShaderProp::TYPE_FLOAT3, 1, DefaultValues::defaultVec3.data);
    checkAndAdd(NMaterialParamName::WATER_CLEAR_COLOR, rhi::ShaderProp::TYPE_FLOAT4, 1, DefaultValues::blackColor.color);

    //checkAndAdd(NMaterialParamName::PARAM_NORMAL_SCALE, rhi::ShaderProp::TYPE_FLOAT1, 1, &DefaultValues::defaultFloat10);
    //checkAndAdd(NMaterialParamName::PARAM_ALPHATEST_THRESHOLD, rhi::ShaderProp::TYPE_FLOAT1, 1, (float32*) &DefaultValues::defaultFloat05);
}
}
