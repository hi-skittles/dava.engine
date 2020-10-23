#include "UI/Components/UIComponent.h"
#include "UI/UIControl.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIComponent)
{
    ReflectionRegistrator<UIComponent>::Begin()
    .End();
}

UIComponent::UIComponent()
    : control(nullptr)
{
}

UIComponent::UIComponent(const UIComponent& src)
    : control(nullptr)
{
}

UIComponent::~UIComponent()
{
}

UIComponent& UIComponent::operator=(const UIComponent& src)
{
    return *this;
}

UIComponent* UIComponent::CreateByType(const Type* componentType)
{
    bool isUIComponent = TypeInheritance::CanDownCast(componentType, Type::Instance<UIComponent>());
    if (isUIComponent)
    {
        const ReflectedType* reflType = ReflectedTypeDB::GetByType(componentType);
        Any obj = reflType->CreateObject(ReflectedType::CreatePolicy::ByPointer);
        return obj.Cast<UIComponent*>();
    }
    else
    {
        throw new std::logic_error("UIComponent::CreateByType can only create UIComponents");
    }
}

RefPtr<UIComponent> UIComponent::SafeCreateByType(const Type* componentType)
{
    return RefPtr<UIComponent>(CreateByType(componentType));
}

RefPtr<UIComponent> UIComponent::SafeClone() const
{
    return RefPtr<UIComponent>(Clone());
}
}
