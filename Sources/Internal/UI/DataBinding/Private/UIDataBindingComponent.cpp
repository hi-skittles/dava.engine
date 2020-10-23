#include "UI/DataBinding/UIDataBindingComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"

ENUM_DECLARE(DAVA::UIDataBindingComponent::UpdateMode)
{
    ENUM_ADD_DESCR(DAVA::UIDataBindingComponent::MODE_READ, "Read");
    ENUM_ADD_DESCR(DAVA::UIDataBindingComponent::MODE_WRITE, "Write");
    ENUM_ADD_DESCR(DAVA::UIDataBindingComponent::MODE_READ_WRITE, "ReadWrite");
};

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIDataBindingComponent)
{
    ReflectionRegistrator<UIDataBindingComponent>::Begin()[M::HiddenField(), M::Multiple(), M::Group("Data")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIDataBindingComponent* o) { o->Release(); })
    .Field("controlField", &UIDataBindingComponent::GetControlFieldName, &UIDataBindingComponent::SetControlFieldName)
    .Field("expression", &UIDataBindingComponent::GetBindingExpression, &UIDataBindingComponent::SetBindingExpression)
    .Field("updateMode", &UIDataBindingComponent::GetUpdateMode, &UIDataBindingComponent::SetUpdateMode)[M::EnumT<UpdateMode>()]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIDataBindingComponent);

UIDataBindingComponent::UIDataBindingComponent()
{
}

UIDataBindingComponent::UIDataBindingComponent(const String& controlField, const String& bindingExpression_, UIDataBindingComponent::UpdateMode mode)
    : controlFieldName(controlField)
    , bindingExpression(bindingExpression_)
    , updateMode(mode)
{
}

UIDataBindingComponent::UIDataBindingComponent(const UIDataBindingComponent& c)
    : controlFieldName(c.controlFieldName)
    , bindingExpression(c.bindingExpression)
    , updateMode(c.updateMode)
{
}

UIDataBindingComponent::~UIDataBindingComponent()
{
}

UIDataBindingComponent* UIDataBindingComponent::Clone() const
{
    return new UIDataBindingComponent(*this);
}

const String& UIDataBindingComponent::GetControlFieldName() const
{
    return controlFieldName;
}

void UIDataBindingComponent::SetControlFieldName(const String& name)
{
    controlFieldName = name;
    isDirty = true;
}

const String& UIDataBindingComponent::GetBindingExpression() const
{
    return bindingExpression;
}

void UIDataBindingComponent::SetBindingExpression(const String& name)
{
    bindingExpression = name;
    isDirty = true;
}

bool UIDataBindingComponent::IsDirty() const
{
    return isDirty;
}

void UIDataBindingComponent::SetDirty(bool dirty_)
{
    isDirty = dirty_;
}

UIDataBindingComponent::UpdateMode UIDataBindingComponent::GetUpdateMode() const
{
    return updateMode;
}

void UIDataBindingComponent::SetUpdateMode(UIDataBindingComponent::UpdateMode mode)
{
    updateMode = mode;
    isDirty = true;
}
}
