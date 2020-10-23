#pragma once 

#include "Base/BaseTypes.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIControl;

/**
    UIMovieView events component.
    Map standard UIMovieView triggers to events.
*/
class UIMovieEventComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIMovieEventComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIMovieEventComponent);

public:
    UIMovieEventComponent();
    UIMovieEventComponent(const UIMovieEventComponent& src);

protected:
    virtual ~UIMovieEventComponent();

private:
    UIMovieEventComponent& operator=(const UIMovieEventComponent&) = delete;

public:
    UIMovieEventComponent* Clone() const override;

    const FastName& GetStartEvent() const;
    void SetStartEvent(const FastName& value);

    const FastName& GetStopEvent() const;
    void SetStopEvent(const FastName& value);

private:
    FastName startEvent;
    FastName stopEvent;
};
}
