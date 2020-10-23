#ifndef __UI_EDITOR_VALUE_PROPERTY__
#define __UI_EDITOR_VALUE_PROPERTY__

#include "AbstractProperty.h"

class ValueProperty : public AbstractProperty
{
    DAVA_VIRTUAL_REFLECTION(ValueProperty, AbstractProperty);

public:
    ValueProperty(const DAVA::String& propName, const DAVA::Type* type);

protected:
    virtual ~ValueProperty();

public:
    DAVA::uint32 GetCount() const override;
    AbstractProperty* GetProperty(DAVA::int32 index) const override;

    void Refresh(DAVA::int32 refreshFlags) override;

    void AttachPrototypeProperty(const ValueProperty* prototypeProperty);
    void DetachPrototypeProperty(const ValueProperty* prototypeProperty);
    const ValueProperty* GetPrototypeProperty() const;
    AbstractProperty* FindPropertyByPrototype(AbstractProperty* prototype) override;

    bool HasChanges() const override;
    const DAVA::String& GetName() const override;
    //ePropertyType GetType() const override;
    DAVA::int32 GetStylePropertyIndex() const override;

    const DAVA::Type* GetValueType() const override;
    void SetValue(const DAVA::Any& newValue) override;
    DAVA::Any GetDefaultValue() const override;
    void SetDefaultValue(const DAVA::Any& newValue) override;
    void ResetValue() override;
    bool IsOverridden() const override;
    bool IsOverriddenLocally() const override;
    bool IsForceOverride() const;
    void SetForceOverride(bool forceOverride);

    virtual const DAVA::Type* GetSubValueType(DAVA::int32 index) const;
    virtual DAVA::Any GetSubValue(DAVA::int32 index) const;
    virtual void SetSubValue(DAVA::int32 index, const DAVA::Any& newValue);
    virtual DAVA::Any GetDefaultSubValue(DAVA::int32 index) const;
    virtual void SetDefaultSubValue(DAVA::int32 index, const DAVA::Any& newValue);

    DAVA::Any ChangeValueComponent(const DAVA::Any& value, const DAVA::Any& component, DAVA::int32 index) const;

protected:
    void GenerateBuiltInSubProperties();

    virtual void ApplyValue(const DAVA::Any& value);
    void SetName(const DAVA::String& newName);
    void SetOverridden(bool overridden);
    void SetStylePropertyIndex(DAVA::int32 index);
    void AddSubValueProperty(AbstractProperty* prop);

private:
    const DAVA::Type* GetValueTypeComponent(DAVA::int32 index) const;
    DAVA::Any GetValueComponent(const DAVA::Any& value, DAVA::int32 index) const;
    bool IsEqual(const DAVA::Any& v1, const DAVA::Any& v2) const;

    DAVA::String name;
    const DAVA::Type* valueType = nullptr;
    DAVA::Any defaultValue;
    DAVA::Vector<DAVA::RefPtr<AbstractProperty>> children;
    DAVA::int32 stylePropertyIndex = -1;
    bool overridden = false;
    bool forceOverride = false;
    const ValueProperty* prototypeProperty = nullptr; // weak
};

#endif //__UI_EDITOR_VALUE_PROPERTY__
