#include "Render/Material/NMaterial.h"
#include "Render/Material/FXCache.h"
#include "Render/Material/NMaterialStateDynamicFlagsInsp.h"

namespace DAVA
{
///////////////////////////////////////////////////////////////////////////
///// NMaterialStateDynamicFlagsInsp implementation

InspInfoDynamic::DynamicData NMaterialStateDynamicFlagsInsp::Prepare(void* object, int filter) const
{
    InspInfoDynamic::DynamicData ddata;
    ddata.object = object;

    return ddata;
}

Vector<FastName> NMaterialStateDynamicFlagsInsp::MembersList(const DynamicData& ddata) const
{
    static Vector<FastName> ret;

    if (ret.empty())
    {
        ret.reserve(26);

        ret.emplace_back(NMaterialFlagName::FLAG_VERTEXFOG);
        ret.emplace_back(NMaterialFlagName::FLAG_FOG_LINEAR);
        ret.emplace_back(NMaterialFlagName::FLAG_FOG_HALFSPACE);
        ret.emplace_back(NMaterialFlagName::FLAG_FOG_HALFSPACE_LINEAR);
        ret.emplace_back(NMaterialFlagName::FLAG_FOG_ATMOSPHERE);

        ret.emplace_back(NMaterialFlagName::FLAG_FLATCOLOR);
        ret.emplace_back(NMaterialFlagName::FLAG_FLATALBEDO);
        ret.emplace_back(NMaterialFlagName::FLAG_TEXTURESHIFT);
        ret.emplace_back(NMaterialFlagName::FLAG_TEXTURE0_ANIMATION_SHIFT);

        ret.emplace_back(NMaterialFlagName::FLAG_WAVE_ANIMATION);
        ret.emplace_back(NMaterialFlagName::FLAG_FAST_NORMALIZATION);

        ret.emplace_back(NMaterialFlagName::FLAG_SPECULAR);
        ret.emplace_back(NMaterialFlagName::FLAG_SEPARATE_NORMALMAPS);
        ret.emplace_back(NMaterialFlagName::FLAG_TANGENT_SPACE_WATER_REFLECTIONS);
        ret.emplace_back(NMaterialFlagName::FLAG_DEBUG_UNITY_Z_NORMAL);
        ret.emplace_back(NMaterialFlagName::FLAG_DEBUG_Z_NORMAL_SCALE);
        ret.emplace_back(NMaterialFlagName::FLAG_DEBUG_NORMAL_ROTATION);
        ret.emplace_back(NMaterialFlagName::FLAG_TEST_OCCLUSION);
        ret.emplace_back(NMaterialFlagName::FLAG_TILED_DECAL_MASK);
        ret.emplace_back(NMaterialFlagName::FLAG_TILED_DECAL_ROTATION);
        ret.emplace_back(NMaterialFlagName::FLAG_ALPHATESTVALUE);
        ret.emplace_back(NMaterialFlagName::FLAG_ALPHASTEPVALUE);
        ret.emplace_back(NMaterialFlagName::FLAG_FORCED_SHADOW_DIRECTION);

        ret.emplace_back(NMaterialFlagName::FLAG_ILLUMINATION_USED);
        ret.emplace_back(NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER);
        ret.emplace_back(NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER);
    }

    return ret;
}

InspDesc NMaterialStateDynamicFlagsInsp::MemberDesc(const DynamicData& ddata, const FastName& member) const
{
    return InspDesc(member.c_str());
}

VariantType NMaterialStateDynamicFlagsInsp::MemberValueGet(const DynamicData& ddata, const FastName& member) const
{
    NMaterial* material = static_cast<NMaterial*>(ddata.object);
    DVASSERT(material);

    VariantType ret;
    ret.SetBool(0 != material->GetEffectiveFlagValue(member));
    return ret;
}

void NMaterialStateDynamicFlagsInsp::MemberValueSet(const DynamicData& ddata, const FastName& member, const VariantType& value)
{
    NMaterial* material = static_cast<NMaterial*>(ddata.object);
    DVASSERT(material);

    // empty value is thread as flag remove
    if (value.GetType() == VariantType::TYPE_NONE)
    {
        if (material->HasLocalFlag(member))
        {
            material->RemoveFlag(member);
        }
    }
    else
    {
        int32 newValue = 0;
        if ((value.GetType() == VariantType::TYPE_BOOLEAN) && value.AsBool())
        {
            newValue = 1;
        }

        if (material->HasLocalFlag(member))
        {
            material->SetFlag(member, newValue);
        }
        else
        {
            material->AddFlag(member, newValue);
        }
    }
}

int NMaterialStateDynamicFlagsInsp::MemberFlags(const DynamicData& ddata, const FastName& member) const
{
    NMaterial* material = static_cast<NMaterial*>(ddata.object);
    DVASSERT(material);

    return I_VIEW | (material->HasLocalFlag(member) ? I_EDIT : 0);
}
};
