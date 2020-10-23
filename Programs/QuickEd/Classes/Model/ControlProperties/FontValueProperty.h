#ifndef __UI_EDITOR_FONT_VALUE_PROPERTY__
#define __UI_EDITOR_FONT_VALUE_PROPERTY__

#include "IntrospectionProperty.h"

class FontValueProperty : public IntrospectionProperty
{
public:
    FontValueProperty(DAVA::BaseObject* object, const DAVA::Type* componentType, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty);
    virtual ~FontValueProperty();

    void Refresh(DAVA::int32 refreshFlags) override;

    DAVA::Any GetValue() const override;

protected:
    void ApplyValue(const DAVA::Any& value) override;

protected:
    DAVA::String presetName;
};

#endif // __UI_EDITOR_FONT_VALUE_PROPERTY__
