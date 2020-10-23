#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIDataChildFactoryComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIDataChildFactoryComponent, UIComponent);

public:
    DECLARE_UI_COMPONENT(UIDataChildFactoryComponent);

    UIDataChildFactoryComponent() = default;
    UIDataChildFactoryComponent(const UIDataChildFactoryComponent& c);

    UIDataChildFactoryComponent& operator=(const UIDataChildFactoryComponent& c) = delete;

    UIDataChildFactoryComponent* Clone() const override;

    const String& GetPackageExpression() const;
    void SetPackageExpression(const String& package);

    const String& GetControlExpression() const;
    void SetControlExpression(const String& control);

    bool IsDirty() const;
    void SetDirty(bool dirty_);

protected:
    ~UIDataChildFactoryComponent() override = default;

private:
    String packageExpression;
    String controlExpression;
    bool isDirty = true;
};
}
