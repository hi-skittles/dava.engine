#ifndef __QUICKED_CLASS_PROPERTY_H__
#define __QUICKED_CLASS_PROPERTY_H__

#include "ValueProperty.h"

class ControlNode;

class ClassProperty : public ValueProperty
{
public:
    ClassProperty(ControlNode* control);

protected:
    virtual ~ClassProperty();

public:
    void Accept(PropertyVisitor* visitor) override;

    bool IsReadOnly() const override;

    ePropertyType GetType() const override;

    DAVA::Any GetValue() const override;
    const DAVA::String& GetClassName() const;
    ControlNode* GetControlNode() const;

private:
    ControlNode* control; // weak
};

#endif // __QUICKED_CLASS_PROPERTY_H__
