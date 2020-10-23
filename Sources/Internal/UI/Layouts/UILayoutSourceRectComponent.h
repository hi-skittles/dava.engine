#pragma once

#include "Math/Vector.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIControl;

/**
     \ingroup layouts
     Layout system uses this component to setup initial control position and size, because real control position and size
     can be changed by layout system in previous invocation. This is useful for QuickEd which uses this component for saving
     position and size entered by user.
     */
class UILayoutSourceRectComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UILayoutSourceRectComponent, UIComponent);
    DECLARE_UI_COMPONENT(UILayoutSourceRectComponent);

public:
    UILayoutSourceRectComponent();
    UILayoutSourceRectComponent(const UILayoutSourceRectComponent& src);
    UILayoutSourceRectComponent* Clone() const override;

    const Vector2& GetPosition() const;
    void SetPosition(const Vector2& position);

    const Vector2& GetSize() const;
    void SetSize(const Vector2& size);

protected:
    virtual ~UILayoutSourceRectComponent();
    UILayoutSourceRectComponent& operator=(const UILayoutSourceRectComponent&) = delete;

private:
    void SetLayoutDirty();

    Vector2 postion;
    Vector2 size;
};
}
