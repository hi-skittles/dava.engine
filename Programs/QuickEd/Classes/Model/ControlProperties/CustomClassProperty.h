#ifndef __QUICKED_CUSTOM_CLASS_PROPERTY_H__
#define __QUICKED_CUSTOM_CLASS_PROPERTY_H__

#include "ValueProperty.h"

class ControlNode;

class CustomClassProperty : public ValueProperty
{
public:
    CustomClassProperty(ControlNode* control, const CustomClassProperty* prototypeProperty);

protected:
    virtual ~CustomClassProperty();

public:
    void Accept(PropertyVisitor* visitor) override;

    bool IsReadOnly() const override;

    ePropertyType GetType() const override;
    DAVA::uint32 GetFlags() const override
    {
        return EF_CAN_RESET;
    };

    DAVA::Any GetValue() const override;

    const DAVA::String& GetCustomClassName() const;

protected:
    virtual void ApplyValue(const DAVA::Any& value) override;

private:
    ControlNode* control; // weak
    DAVA::String customClass;
};

#endif // __QUICKED_CUSTOM_CLASS_PROPERTY_H__
