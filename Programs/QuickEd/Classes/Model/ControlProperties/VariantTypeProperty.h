#ifndef __QUICKED_VALUE_PROPERTY_H__
#define __QUICKED_VALUE_PROPERTY_H__

#include "Model/ControlProperties/ValueProperty.h"
#include <Reflection/ReflectedStructure.h>

class ValueProperty;

class StyleSheetNode;

namespace DAVA
{
class UIControl;
}

class VariantTypeProperty : public ValueProperty
{
public:
    VariantTypeProperty(const DAVA::String& name, DAVA::Any& variantType, const DAVA::ReflectedStructure::Field* field);

protected:
    virtual ~VariantTypeProperty();

public:
    void Accept(PropertyVisitor* visitor) override;
    bool IsReadOnly() const override;

    ePropertyType GetType() const override;
    const EnumMap* GetEnumMap() const override;
    DAVA::Any GetValue() const override;
    void ApplyValue(const DAVA::Any& value) override;

private:
    DAVA::Any& value;
    const DAVA::ReflectedStructure::Field* field = nullptr;
};

#endif // __QUICKED_VALUE_PROPERTY_H__
