#include "UI/DataBinding/UIDataSourceComponent.h"

#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Utils/StringUtils.h"

ENUM_DECLARE(DAVA::UIDataSourceComponent::eSourceType)
{
    ENUM_ADD_DESCR(DAVA::UIDataSourceComponent::FROM_REFLECTION, "Reflection");
    ENUM_ADD_DESCR(DAVA::UIDataSourceComponent::FROM_FILE, "File");
    ENUM_ADD_DESCR(DAVA::UIDataSourceComponent::FROM_EXPRESSION, "Expression");
};

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIDataSourceComponent)
{
    ReflectionRegistrator<UIDataSourceComponent>::Begin()[M::DisplayName("Data Source"), M::Group("Data"), M::Multiple()]
    .ConstructorByPointer()
    .DestructorByPointer([](UIDataSourceComponent* o) { o->Release(); })
    .Field("sourceType", &UIDataSourceComponent::GetSourceType, &UIDataSourceComponent::SetSourceType)[M::EnumT<eSourceType>(), M::DisplayName("Source Type")]
    .Field("source", &UIDataSourceComponent::GetSource, &UIDataSourceComponent::SetSource)[M::DisplayName("Source")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIDataSourceComponent);

UIDataSourceComponent::UIDataSourceComponent(const UIDataSourceComponent& c)
    : sourceType(c.sourceType)
    , data(c.data)
    , source(c.source)
{
    isDirty = true;
}

UIDataSourceComponent* UIDataSourceComponent::Clone() const
{
    return new UIDataSourceComponent(*this);
}

UIDataSourceComponent::eSourceType UIDataSourceComponent::GetSourceType() const
{
    return sourceType;
}

void UIDataSourceComponent::SetSourceType(eSourceType sourceType_)
{
    sourceType = sourceType_;
    isDirty = true;
}

const Reflection& UIDataSourceComponent::GetData() const
{
    return data;
}

void UIDataSourceComponent::SetData(const Reflection& data_)
{
    data = data_;
    isDirty = true;
}

const String& UIDataSourceComponent::GetSource() const
{
    return source;
}

void UIDataSourceComponent::SetSource(const String& source_)
{
    source = source_;
    isDirty = true;
}

bool UIDataSourceComponent::IsDirty() const
{
    return isDirty;
}

void UIDataSourceComponent::SetDirty(bool dirty_)
{
    isDirty = dirty_;
}
}
