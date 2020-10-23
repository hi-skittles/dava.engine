#pragma once

#include "Base/BaseTypes.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIDataBindingComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIDataBindingComponent, UIComponent);

public:
    DECLARE_UI_COMPONENT(UIDataBindingComponent);
    enum UpdateMode
    {
        MODE_READ = 0, //!< Read data from model and put in UI
        MODE_WRITE = 1, //!< Get data from UI and write it in model
        MODE_READ_WRITE = 2 //!< First get data from UI and write to model, after update read data from model and put it in UI
    };

    UIDataBindingComponent();
    UIDataBindingComponent(const String& controlField, const String& bindingExpression, UpdateMode mode = MODE_WRITE);
    UIDataBindingComponent(const UIDataBindingComponent& c);

    UIDataBindingComponent& operator=(const UIDataBindingComponent& c) = delete;
    UIDataBindingComponent* Clone() const override;

    const String& GetControlFieldName() const;
    void SetControlFieldName(const String& name);

    const String& GetBindingExpression() const;
    void SetBindingExpression(const String& name);

    bool IsDirty() const;
    void SetDirty(bool dirty);

    UpdateMode GetUpdateMode() const;
    void SetUpdateMode(UpdateMode mode);

protected:
    ~UIDataBindingComponent() override;

private:
    String controlFieldName;
    String bindingExpression;
    UpdateMode updateMode = MODE_READ;

    bool isDirty = true;
};
}
