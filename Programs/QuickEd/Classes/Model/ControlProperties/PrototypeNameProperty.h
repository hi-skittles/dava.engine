#ifndef __UI_EDITOR_PROTOTYPE_NAME_PROPERTY__
#define __UI_EDITOR_PROTOTYPE_NAME_PROPERTY__

#include "ValueProperty.h"

class ControlNode;

class PrototypeNameProperty : public ValueProperty
{
public:
    PrototypeNameProperty(ControlNode* aNode, const PrototypeNameProperty* prototypeProperty);

protected:
    virtual ~PrototypeNameProperty();

public:
    void Accept(PropertyVisitor* visitor) override;

    ePropertyType GetType() const override;
    DAVA::Any GetValue() const override;
    bool IsReadOnly() const override;
    DAVA::String GetPrototypeName() const;

    ControlNode* GetControl() const;

protected:
    void ApplyValue(const DAVA::Any& value) override;

private:
    ControlNode* node; // weak
};

#endif //__UI_EDITOR_PROTOTYPE_NAME_PROPERTY__
