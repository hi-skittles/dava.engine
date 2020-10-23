#ifndef __QUICKED_VISIBLE_VALUE_PROPERTY__
#define __QUICKED_VISIBLE_VALUE_PROPERTY__

#include "IntrospectionProperty.h"

class VisibleValueProperty : public IntrospectionProperty
{
public:
    VisibleValueProperty(DAVA::BaseObject* object, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty);
    ~VisibleValueProperty() override = default;

    void SetVisibleInEditor(bool visible);
    bool GetVisibleInEditor() const;
};

#endif // __QUICKED_VISIBLE_VALUE_PROPERTY__
