#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIControlSourceComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIControlSourceComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIControlSourceComponent);

public:
    UIControlSourceComponent();
    UIControlSourceComponent(const UIControlSourceComponent& src);
    UIControlSourceComponent& operator=(const UIControlSourceComponent& src) = delete;
    UIControlSourceComponent* Clone() const override;

    void SetPackagePath(const String& path);
    const String& GetPackagePath() const;

    void SetControlName(const String& name);
    const String& GetControlName() const;

    void SetPrototypeName(const String& name);
    const String& GetPrototypeName() const;

protected:
    ~UIControlSourceComponent() override;

private:
    String packagePath;
    String controlName;
    String prototypeName;
};

inline const String& UIControlSourceComponent::GetPackagePath() const
{
    return packagePath;
}

inline const String& UIControlSourceComponent::GetControlName() const
{
    return controlName;
}

inline const String& UIControlSourceComponent::GetPrototypeName() const
{
    return prototypeName;
}
}
