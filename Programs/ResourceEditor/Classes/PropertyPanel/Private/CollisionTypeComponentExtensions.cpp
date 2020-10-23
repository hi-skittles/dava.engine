#include "Classes/PropertyPanel/PropertyPanelCommon.h"
#include "Classes/PropertyPanel/CollisionTypeComponentExtensions.h"

#include <REPlatform/Scene/Components/CollisionTypeComponent.h>
#include <REPlatform/DataNodes/DebugDrawModuleData.h>


#include <TArc/Controls/ComboBox.h>
#include <TArc/Utils/ReflectionHelpers.h>
#include <TArc/Utils/ReflectedPairsVector.h>

using CollisionTypesMap = DAVA::ReflectedPairsVector<DAVA::int32, DAVA::String>;

class CollisionTypeNameValue final : public DAVA::BaseComponentValue
{
public:
    CollisionTypeNameValue(bool crashed)
        : crashed(crashed)
    {
    }

    DAVA::Any GetMultipleValue() const override
    {
        return DAVA::Any();
    }

    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override
    {
        if (currentValue.IsEmpty())
        {
            return true;
        }

        return newValue.IsEmpty() == true || newValue != currentValue;
    }

    DAVA::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::DataWrappersProcessor* wrappersProcessor) override
    {
        DAVA::ContextAccessor* accessor = GetAccessor();
        DAVA::DebugDrawModuleData* data = accessor->GetGlobalContext()->GetData<DAVA::DebugDrawModuleData>();
        const CollisionTypesMap& map = data->GetCollisionTypes();
        if (crashed == false)
        {
            enumerator.values.clear();
            for (auto pair : map.values)
            {
                if (pair.first >= 0)
                {
                    enumerator.values.push_back(pair);
                }
            }
        }
        else
        {
            enumerator.values = data->GetCollisionTypesCrashed().values;
        }
        std::pair<DAVA::int32, DAVA::String> undefinedCollision =
        *std::find_if(std::begin(map.values), std::end(map.values),
                      [](const std::pair<DAVA::int32, DAVA::String>& pair) {
                          return pair.first == DAVA::CollisionTypeValues::COLLISION_TYPE_UNDEFINED;
                      });
        enumerator.values.push_back(undefinedCollision);

        DAVA::ComboBox::Params params(accessor, GetUI(), GetWindowKey());
        params.fields[DAVA::ComboBox::Fields::Enumerator] = "enumerator";
        params.fields[DAVA::ComboBox::Fields::Value] = "value";

        return new DAVA::ComboBox(params, wrappersProcessor, model, parent);
    }

private:
    DAVA::Any GetValueIndex() const
    {
        DAVA::Any index = GetValue();
        if (index.IsEmpty())
        {
            return index;
        }
        return index.Cast<DAVA::int32>();
    }

    void SetValueIndex(const DAVA::Any& optionValue)
    {
        SetValue(optionValue);
    }

    CollisionTypesMap enumerator;
    bool crashed = false;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(CollisionTypeNameValue, DAVA::BaseComponentValue)
    {
        DAVA::ReflectionRegistrator<CollisionTypeNameValue>::Begin()
        .Field("enumerator", &CollisionTypeNameValue::enumerator)
        .Field("value", &CollisionTypeNameValue::GetValueIndex, &CollisionTypeNameValue::SetValueIndex)
        .End();
    }
};

void PropertyPanel::CollisionTypeChildCreator::ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const
{
    DAVA::ChildCreatorExtension::ExposeChildren(parent, children);
    const DAVA::ReflectedType* fieldType = DAVA::GetValueReflectedType(parent->field.ref);
    const DAVA::ReflectedType* collisionTypeComponentType = DAVA::ReflectedTypeDB::Get<DAVA::CollisionTypeComponent>();
    if (fieldType != collisionTypeComponentType)
        return;

    auto setPropertyType = [this, &children](const DAVA::String& name, eREPropertyType type)
    {
        auto iter =
        std::find_if(std::begin(children), std::end(children),
                     [&name](const std::shared_ptr<DAVA::PropertyNode>& node)
                     {
                         return node->field.key.Cast<DAVA::String>() == name;
                     });

        DVASSERT(iter != std::end(children));
        (*iter)->propertyType = type;
    };

    setPropertyType(DAVA::CollisionTypeComponent::CollisionTypeFieldName, CollisionPropertyType);
    setPropertyType(DAVA::CollisionTypeComponent::CollisionTypeCrashedFieldName, CollisionPropertyTypeCrashed);
}

std::unique_ptr<DAVA::BaseComponentValue> PropertyPanel::CollisionTypeEditorCreator::GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const
{
    if (node->propertyType == CollisionPropertyType)
    {
        return std::make_unique<CollisionTypeNameValue>(false);
    }

    if (node->propertyType == CollisionPropertyTypeCrashed)
    {
        return std::make_unique<CollisionTypeNameValue>(true);
    }

    return EditorComponentExtension::GetEditor(node);
}
