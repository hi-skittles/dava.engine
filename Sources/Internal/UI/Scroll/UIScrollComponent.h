#pragma once

#include "Base/BaseTypes.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIScrollComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIScrollComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIScrollComponent);

public:
    UIScrollComponent();
    UIScrollComponent(const UIScrollComponent& src);

protected:
    virtual ~UIScrollComponent();

    UIScrollComponent& operator=(const UIScrollComponent&) = delete;

public:
    UIScrollComponent* Clone() const override;
};
}
