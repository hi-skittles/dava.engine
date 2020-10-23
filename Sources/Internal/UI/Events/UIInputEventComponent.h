#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIControl;

class UIInputEventComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIInputEventComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIInputEventComponent);

public:
    UIInputEventComponent();
    UIInputEventComponent(const UIInputEventComponent& src);

protected:
    virtual ~UIInputEventComponent();

private:
    UIInputEventComponent& operator=(const UIInputEventComponent&) = delete;

public:
    UIInputEventComponent* Clone() const override;

    const FastName& GetOnTouchDownEvent() const;
    void SetOnTouchDownEvent(const FastName& value);

    const String& GetOnTouchDownDataExpression() const;
    void SetOnTouchDownDataExpression(const String& exp);

    const FastName& GetOnTouchUpInsideEvent() const;
    void SetOnTouchUpInsideEvent(const FastName& value);

    const String& GetOnTouchUpInsideDataExpression() const;
    void SetOnTouchUpInsideDataExpression(const String& exp);

    const FastName& GetOnTouchUpOutsideEvent() const;
    void SetOnTouchUpOutsideEvent(const FastName& value);

    const String& GetOnTouchUpOutsideDataExpression() const;
    void SetOnTouchUpOutsideDataExpression(const String& exp);

    const FastName& GetOnValueChangedEvent() const;
    void SetOnValueChangedEvent(const FastName& value);

    const String& GetOnValueChangedDataExpression() const;
    void SetOnValueChangedDataExpression(const String& exp);

    const FastName& GetOnHoverSetEvent() const;
    void SetOnHoverSetEvent(const FastName& value);

    const String& GetOnHoverSetDataExpression() const;
    void SetOnHoverSetDataExpression(const String& exp);

    const FastName& GetOnHoverRemovedEvent() const;
    void SetOnHoverRemovedEvent(const FastName& value);

    const String& GetOnHoverRemovedDataExpression() const;
    void SetOnHoverRemovedDataExpression(const String& exp);

private:
    FastName onTouchDown;
    String onTouchDownDataExpression;

    FastName onTouchUpInside;
    String onTouchUpInsideDataExpression;

    FastName onTouchUpOutside;
    String onTouchUpOutsideDataExpression;

    FastName onValueChanged;
    String onValueChangedDataExpression;

    FastName onHoverSet;
    String onHoverSetDataExpression;

    FastName onHoverRemoved;
    String onHoverRemovedDataExpression;
};
}
