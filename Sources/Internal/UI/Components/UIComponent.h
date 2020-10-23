#pragma once

#include "Base/BaseObject.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIControl;

class UIComponent : public BaseObject
{
    DAVA_VIRTUAL_REFLECTION(UIComponent, BaseObject);

public:
    UIComponent();
    UIComponent(const UIComponent& src);

    UIComponent& operator=(const UIComponent& src);

    static UIComponent* CreateByType(const Type* componentType);
    static RefPtr<UIComponent> SafeCreateByType(const Type* componentType);

    void SetControl(UIControl* _control);
    UIControl* GetControl() const;

    virtual UIComponent* Clone() const = 0;

    RefPtr<UIComponent> SafeClone() const;

    virtual int32 GetRuntimeType() const = 0;

    virtual const Type* GetType() const = 0;

protected:
    virtual ~UIComponent();

private:
    UIControl* control;
};

inline void UIComponent::SetControl(UIControl* _control)
{
    control = _control;
}

inline UIControl* UIComponent::GetControl() const
{
    return control;
}

// clang-format off
#define DECLARE_UI_COMPONENT(TYPE) \
    const DAVA::Type* GetType() const override; \
    DAVA::int32 GetRuntimeType() const override; \

#define IMPLEMENT_UI_COMPONENT(TYPE) \
    const DAVA::Type* TYPE::GetType() const { return DAVA::Type::Instance<TYPE>(); }; \
    DAVA::int32 TYPE::GetRuntimeType() const \
    { \
        static DAVA::int32 runtimeType = DAVA::GetEngineContext()->componentManager->GetRuntimeComponentId(GetType()); \
        return runtimeType; \
    }
// clang-format on
}
