#ifndef __DAVAENGINE_NMATERIALSTATEDYNAMICPROPERTYSINSP_NAMES_H__
#define __DAVAENGINE_NMATERIALSTATEDYNAMICPROPERTYSINSP_NAMES_H__

#include "Base/BaseTypes.h"
#include "Base/Introspection.h"
#include "Render/RHI/rhi_ShaderSource.h"

namespace DAVA
{
class NMaterial;

class NMaterialStateDynamicPropertiesInsp : public InspInfoDynamic
{
public:
    DynamicData Prepare(void* object, int filter) const override;
    Vector<FastName> MembersList(const DynamicData& ddata) const override;
    InspDesc MemberDesc(const DynamicData& ddata, const FastName& key) const override;
    int MemberFlags(const DynamicData& ddata, const FastName& key) const override;
    VariantType MemberValueGet(const DynamicData& ddata, const FastName& key) const override;
    void MemberValueSet(const DynamicData& ddata, const FastName& key, const VariantType& value) override;

private:
    struct PropData
    {
        uint32 size;
        rhi::ShaderProp::Type type;
        const float32* defaultValue;
    };
    using PropDataMap = UnorderedMap<FastName, PropData>;
    bool IsColor(const FastName& key) const;
    void FindMaterialPropertiesRecursive(NMaterial* state, PropDataMap& props) const;
    void FillGlobalMaterialMemebers(NMaterial* state, PropDataMap& props) const;
};
};

#endif /* defined(__DAVAENGINE_NMATERIALSTATEDYNAMICPROPERTYSINSP_NAMES_H__) */
