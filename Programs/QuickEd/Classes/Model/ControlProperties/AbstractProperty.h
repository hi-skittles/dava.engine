#ifndef __UI_EDITOR_ABSTRACT_PROPERTY_H__
#define __UI_EDITOR_ABSTRACT_PROPERTY_H__

#include <Base/Any.h>
#include <Base/BaseObject.h>
#include <Base/Type.h>

class PropertyVisitor;

class AbstractProperty : public DAVA::BaseObject
{
    DAVA_VIRTUAL_REFLECTION(AbstractProperty, DAVA::BaseObject);

public:
    enum ePropertyType
    {
        TYPE_NONE,
        TYPE_HEADER,
        TYPE_VARIANT,
        TYPE_ENUM,
        TYPE_FLAGS,
    };

    enum eEditFrags
    {
        EF_NONE = 0x00,
        EF_CAN_RESET = 0x01,
        EF_INHERITED = 0x02,
        EF_CAN_REMOVE = 0x04,
    };

    enum eRefreshFlags
    {
        REFRESH_DEFAULT_VALUE = 0x01,
        REFRESH_LOCALIZATION = 0x02,
        REFRESH_FONT = 0x04,
        REFRESH_ALL = REFRESH_FONT | REFRESH_LOCALIZATION | REFRESH_DEFAULT_VALUE
    };

public:
    AbstractProperty();

protected:
    virtual ~AbstractProperty();

public:
    AbstractProperty* GetParent() const;
    void SetParent(AbstractProperty* parent);

    virtual DAVA::uint32 GetCount() const = 0;
    virtual AbstractProperty* GetProperty(DAVA::int32 index) const = 0;
    virtual DAVA::int32 GetIndex(const AbstractProperty* property) const;

    virtual void Refresh(DAVA::int32 refreshFlags);
    virtual AbstractProperty* FindPropertyByPrototype(AbstractProperty* prototype);
    virtual AbstractProperty* FindPropertyByStyleIndex(DAVA::int32 propertyIndex) const;
    virtual bool HasChanges() const;
    virtual void Accept(PropertyVisitor* visitor) = 0;

    virtual const DAVA::String& GetName() const = 0;
    virtual const DAVA::String& GetDisplayName() const;
    virtual ePropertyType GetType() const = 0;
    virtual DAVA::uint32 GetFlags() const;
    virtual DAVA::int32 GetStylePropertyIndex() const;

    virtual bool IsReadOnly() const;

    virtual const DAVA::Type* GetValueType() const = 0;
    virtual DAVA::Any GetValue() const;
    virtual DAVA::Any GetSerializationValue() const;
    virtual void SetValue(const DAVA::Any& newValue);
    virtual DAVA::Any GetDefaultValue() const;
    virtual void SetDefaultValue(const DAVA::Any& newValue);
    virtual const EnumMap* GetEnumMap() const;
    virtual void ResetValue();
    virtual bool IsOverriddenLocally() const;
    virtual bool IsOverridden() const;
    virtual bool IsBindable() const;
    virtual bool IsBound() const;
    virtual bool HasError() const;
    virtual DAVA::String GetErrorString() const;
    virtual DAVA::int32 GetBindingUpdateMode() const;
    virtual DAVA::String GetBindingExpression() const;
    virtual void SetBindingExpression(const DAVA::String& expression, DAVA::int32 bindingUpdateMode);

    AbstractProperty* GetRootProperty();
    const AbstractProperty* GetRootProperty() const;

    AbstractProperty* FindPropertyByName(const DAVA::String& name);

private:
    AbstractProperty* parent = nullptr;
};


#endif // __UI_EDITOR_ABSTRACT_PROPERTY_H__
