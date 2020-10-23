#include "UI/Events/UIMovieEventComponent.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIMovieEventComponent)
{
    ReflectionRegistrator<UIMovieEventComponent>::Begin()[M::DisplayName("Movie Event"), M::Group("Input")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIMovieEventComponent* o) { o->Release(); })
    .Field("onStart", &UIMovieEventComponent::GetStartEvent, &UIMovieEventComponent::SetStartEvent)[M::DisplayName("Start")]
    .Field("onStop", &UIMovieEventComponent::GetStopEvent, &UIMovieEventComponent::SetStopEvent)[M::DisplayName("Stop")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIMovieEventComponent);

UIMovieEventComponent::UIMovieEventComponent()
{
}

UIMovieEventComponent::UIMovieEventComponent(const UIMovieEventComponent& src)
    : startEvent(src.startEvent)
    ,
    stopEvent(src.stopEvent)
{
}

UIMovieEventComponent::~UIMovieEventComponent()
{
}

UIMovieEventComponent* UIMovieEventComponent::Clone() const
{
    return new UIMovieEventComponent(*this);
}

const FastName& UIMovieEventComponent::GetStartEvent() const
{
    return startEvent;
}

void UIMovieEventComponent::SetStartEvent(const FastName& value)
{
    startEvent = value;
}

const FastName& UIMovieEventComponent::GetStopEvent() const
{
    return stopEvent;
}

void UIMovieEventComponent::SetStopEvent(const FastName& value)
{
    stopEvent = value;
}
}
