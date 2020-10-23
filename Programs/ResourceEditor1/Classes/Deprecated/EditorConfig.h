#ifndef __EDITOR_CONFIG_H__
#define __EDITOR_CONFIG_H__

#include "DAVAEngine.h"

class PropertyDescription : public DAVA::BaseObject
{
protected:
    ~PropertyDescription()
    {
    }

public:
    PropertyDescription()
        : DAVA::BaseObject()
        , type(0){};

    DAVA::String name;
    DAVA::int32 type;
    DAVA::VariantType defaultValue;
    DAVA::Vector<DAVA::String> comboValues;
    DAVA::Vector<DAVA::Color> colorListValues;
};

class EditorConfig
{
public:
    EditorConfig();
    virtual ~EditorConfig();

    enum ePropertyType
    {
        PT_NONE = 0,
        PT_BOOL,
        PT_INT,
        PT_FLOAT,
        PT_STRING,
        PT_COMBOBOX,
        PT_COLOR_LIST,

        PROPERTY_TYPES_COUNT
    };

    void ParseConfig(const DAVA::FilePath& filePath);

    const DAVA::Vector<DAVA::String>& GetProjectPropertyNames() const;
    const DAVA::Vector<DAVA::String>& GetComboPropertyValues(const DAVA::String& nameStr) const;
    const DAVA::Vector<DAVA::Color>& GetColorPropertyValues(const DAVA::String& nameStr) const;

    bool HasProperty(const DAVA::String& propertyName) const;
    DAVA::int32 GetPropertyValueType(const DAVA::String& propertyName) const;
    const DAVA::VariantType* GetPropertyDefaultValue(const DAVA::String& propertyName) const;

protected:
    void ClearConfig();

    const PropertyDescription* GetPropertyDescription(const DAVA::String& propertyName) const;

    DAVA::int32 GetValueTypeFromPropertyType(DAVA::int32 propertyType) const;
    DAVA::int32 ParseType(const DAVA::String& typeStr);

    DAVA::Vector<DAVA::String> propertyNames;
    DAVA::Map<DAVA::String, PropertyDescription*> properties;
    DAVA::Vector<DAVA::String> empty;
    DAVA::Vector<DAVA::Color> emptyColors;
};



#endif // __EDITOR_CONFIG_H__