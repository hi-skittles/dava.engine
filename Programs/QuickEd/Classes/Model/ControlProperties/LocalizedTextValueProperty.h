#ifndef __UI_EDITOR_LOCALIZED_TEXT_VALUE_PROPERTY__
#define __UI_EDITOR_LOCALIZED_TEXT_VALUE_PROPERTY__

#include "IntrospectionProperty.h"

class LocalizedTextValueProperty : public IntrospectionProperty
{
public:
    LocalizedTextValueProperty(DAVA::BaseObject* object, const DAVA::Type* componentType, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty);

protected:
    virtual ~LocalizedTextValueProperty();

public:
    void Refresh(DAVA::int32 refreshFlags) override;

    DAVA::Any GetValue() const override;

protected:
    void ApplyValue(const DAVA::Any& value) override;

protected:
    DAVA::String text;
};

#endif // __UI_EDITOR_LOCALIZED_TEXT_VALUE_PROPERTY__
