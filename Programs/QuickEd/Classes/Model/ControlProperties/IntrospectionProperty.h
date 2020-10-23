#ifndef __UI_EDITOR_INTROSPECTION_PROPERTY__
#define __UI_EDITOR_INTROSPECTION_PROPERTY__

#include "ValueProperty.h"

namespace DAVA
{
class UIControl;
class UIDataBindingComponent;
class UILayoutSourceRectComponent;
}

class IntrospectionProperty : public ValueProperty
{
public:
    IntrospectionProperty(DAVA::BaseObject* object, const DAVA::Type* componentType, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty);

protected:
    IntrospectionProperty(DAVA::BaseObject* object, DAVA::int32 componentType, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty);
    virtual ~IntrospectionProperty();

public:
    static IntrospectionProperty* Create(DAVA::BaseObject* object, const DAVA::Type* componentType, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* sourceProperty);

    void Accept(PropertyVisitor* visitor) override;

    DAVA::uint32 GetFlags() const override;

    ePropertyType GetType() const override;
    const EnumMap* GetEnumMap() const override;

    const DAVA::String& GetDisplayName() const override;
    DAVA::Any GetValue() const override;
    DAVA::Any GetSerializationValue() const override;

    DAVA::BaseObject* GetBaseObject() const
    {
        return object;
    }

    void DisableResetFeature();

    void ResetValue() override;
    void Refresh(DAVA::int32 refreshFlags) override;
    bool IsBindable() const override;
    bool IsBound() const override;
    DAVA::int32 GetBindingUpdateMode() const override;
    DAVA::String GetBindingExpression() const override;
    void SetBindingExpression(const DAVA::String& expression, DAVA::int32 bindingUpdateMode) override;
    DAVA::String GetFullFieldName() const;

    bool HasError() const override;
    DAVA::String GetErrorString() const override;

    bool IsReadOnly() const override;

    void ComponentWithPropertyWasInstalled();
    void ComponentWithPropertyWasUninstalled();

protected:
    void SetBindingExpressionImpl(const DAVA::String& expression, DAVA::int32 bindingUpdateMode);
    void ResetBindingExpression();
    void ApplyValue(const DAVA::Any& value) override;

    DAVA::BaseObject* object = nullptr;
    DAVA::Reflection reflection;
    DAVA::int32 flags;
    DAVA::String bindingExpression;
    DAVA::int32 bindingMode = 0;
    bool forceReadOnly = false;

private:
    DAVA::UIControl* GetLinkedControl();
    bool bindable = false;
    bool bound = false;
    const DAVA::Type* componentType = nullptr;
    DAVA::RefPtr<DAVA::UIDataBindingComponent> bindingComponent;
    void SetLayoutSourceRectValue(const DAVA::Any& value);
    DAVA::RefPtr<DAVA::UILayoutSourceRectComponent> sourceRectComponent;
};

#endif //__UI_EDITOR_INTROSPECTION_PROPERTY__
