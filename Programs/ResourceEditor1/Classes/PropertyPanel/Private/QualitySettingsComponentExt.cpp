#include "Classes/PropertyPanel/QualitySettingsComponentExt.h"
#include "Classes/PropertyPanel/PropertyPanelCommon.h"
#include "Classes/PropertyPanel/Private/QualityGroupComponentValue.h"

#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Utils/ReflectionHelpers.h>

#include <Scene3D/Components/QualitySettingsComponent.h>
#include <Reflection/ReflectedType.h>
#include <Reflection/ReflectedTypeDB.h>

void QualitySettingsChildCreator::ExposeChildren(const std::shared_ptr<DAVA::TArc::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::TArc::PropertyNode>>& children) const
{
    using namespace DAVA;
    using namespace DAVA::TArc;

    static const ReflectedType* qualityComponentType = ReflectedTypeDB::Get<QualitySettingsComponent>();
    if (GetValueReflectedType(parent->cachedValue) == qualityComponentType)
    {
        if (parent->propertyType != PropertyPanel::GroupQualityProperty)
        {
            Reflection::Field field = parent->field;
            field.key = "group_quality";
            children.push_back(allocator->CreatePropertyNode(parent, std::move(field), static_cast<int32>(children.size()),
                                                             PropertyPanel::GroupQualityProperty, parent->cachedValue));
        }
    }
    else
    {
        ChildCreatorExtension::ExposeChildren(parent, children);
    }
}

std::unique_ptr<DAVA::TArc::BaseComponentValue> QualitySettingsEditorCreator::GetEditor(const std::shared_ptr<const DAVA::TArc::PropertyNode>& node) const
{
    if (node->propertyType == PropertyPanel::GroupQualityProperty)
    {
        return std::make_unique<QualityGroupComponentValue>();
    }

    return EditorComponentExtension::GetEditor(node);
}
