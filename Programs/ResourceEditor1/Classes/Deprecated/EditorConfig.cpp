#include "Classes/Deprecated/EditorConfig.h"

EditorConfig::EditorConfig()
{
    empty.push_back("none");
}

EditorConfig::~EditorConfig()
{
    ClearConfig();
}

void EditorConfig::ClearConfig()
{
    propertyNames.clear();
    DAVA::Map<DAVA::String, PropertyDescription*>::iterator it = properties.begin();
    DAVA::Map<DAVA::String, PropertyDescription*>::iterator propEnd = properties.end();
    for (; it != propEnd; ++it)
    {
        SafeRelease(it->second);
    }
    properties.clear();
}

DAVA::int32 EditorConfig::ParseType(const DAVA::String& typeStr)
{
    if (typeStr == "Bool")
    {
        return PT_BOOL;
    }
    if (typeStr == "Int")
    {
        return PT_INT;
    }
    if (typeStr == "Float")
    {
        return PT_FLOAT;
    }

    if (typeStr == "String")
    {
        return PT_STRING;
    }
    if (typeStr == "Combobox")
    {
        return PT_COMBOBOX;
    }

    if (typeStr == "ColorList")
    {
        return PT_COLOR_LIST;
    }
    return PT_NONE;
}

void EditorConfig::ParseConfig(const DAVA::FilePath& filePath)
{
    ClearConfig();

    DAVA::YamlParser* parser = DAVA::YamlParser::Create(filePath);
    if (parser)
    {
        DAVA::YamlNode* rootNode = parser->GetRootNode();
        if (rootNode)
        {
            const DAVA::Vector<DAVA::YamlNode*>& yamlNodes = rootNode->AsVector();
            size_t propertiesCount = yamlNodes.size();
            for (size_t i = 0; i < propertiesCount; ++i)
            {
                DAVA::YamlNode* propertyNode = yamlNodes[i];
                if (propertyNode)
                {
                    const DAVA::YamlNode* nameNode = propertyNode->Get("name");
                    const DAVA::YamlNode* typeNode = propertyNode->Get("type");
                    const DAVA::YamlNode* defaultNode = propertyNode->Get("default");
                    if (nameNode && typeNode)
                    {
                        const DAVA::String& nameStr = nameNode->AsString();
                        const DAVA::String& typeStr = typeNode->AsString();
                        DAVA::int32 type = ParseType(typeStr);
                        if (type)
                        {
                            bool isOk = true;
                            for (const DAVA::String& propertyName : propertyNames)
                            {
                                if (propertyName == nameStr)
                                {
                                    isOk = false;
                                    DAVA::Logger::Error("EditorConfig::ParseConfig %s ERROR property %d property %s already exists", filePath.GetAbsolutePathname().c_str(), i, nameStr.c_str());
                                    break;
                                }
                            }

                            if (isOk)
                            {
                                properties[nameStr] = new PropertyDescription();
                                properties[nameStr]->name = nameStr;
                                properties[nameStr]->type = type;
                                switch (type)
                                {
                                case PT_BOOL:
                                {
                                    bool defaultValue = false;
                                    if (defaultNode)
                                    {
                                        defaultValue = defaultNode->AsBool();
                                    }
                                    properties[nameStr]->defaultValue.SetBool(defaultValue);
                                }
                                break;
                                case PT_INT:
                                {
                                    DAVA::int32 defaultValue = 0;
                                    if (defaultNode)
                                    {
                                        defaultValue = defaultNode->AsInt();
                                    }
                                    properties[nameStr]->defaultValue.SetInt32(defaultValue);
                                }
                                break;
                                case PT_STRING:
                                {
                                    DAVA::String defaultValue;
                                    if (defaultNode)
                                    {
                                        defaultValue = defaultNode->AsString();
                                    }
                                    properties[nameStr]->defaultValue.SetString(defaultValue);
                                }
                                break;
                                case PT_FLOAT:
                                {
                                    DAVA::float32 defaultValue = 0.0f;
                                    if (defaultNode)
                                    {
                                        defaultValue = defaultNode->AsFloat();
                                    }
                                    properties[nameStr]->defaultValue.SetFloat(defaultValue);
                                }
                                break;
                                case PT_COMBOBOX:
                                {
                                    DAVA::int32 defaultValue = 0;
                                    if (defaultNode)
                                    {
                                        defaultValue = defaultNode->AsInt();
                                    }
                                    properties[nameStr]->defaultValue.SetInt32(defaultValue);

                                    const DAVA::YamlNode* comboNode = propertyNode->Get("list");
                                    if (comboNode)
                                    {
                                        const DAVA::Vector<DAVA::YamlNode*>& comboValueNodes = comboNode->AsVector();
                                        for (const DAVA::YamlNode* comboValueNode : comboValueNodes)
                                        {
                                            properties[nameStr]->comboValues.push_back(comboValueNode->AsString());
                                        }
                                    }
                                }
                                break;
                                case PT_COLOR_LIST:
                                {
                                    DAVA::int32 defaultValue = 0;
                                    if (defaultNode)
                                    {
                                        defaultValue = defaultNode->AsInt();
                                    }
                                    properties[nameStr]->defaultValue.SetInt32(defaultValue);

                                    const DAVA::YamlNode* colorListNode = propertyNode->Get("list");
                                    if (colorListNode)
                                    {
                                        const DAVA::Vector<DAVA::YamlNode*>& colorListNodes = colorListNode->AsVector();
                                        for (const DAVA::YamlNode* colorNode : colorListNodes)
                                        {
                                            if (!colorNode || colorNode->GetCount() != 4)
                                                continue;

                                            DAVA::Color color(colorNode->Get(0)->AsFloat() / 255.f,
                                                              colorNode->Get(1)->AsFloat() / 255.f,
                                                              colorNode->Get(2)->AsFloat() / 255.f,
                                                              colorNode->Get(3)->AsFloat() / 255.f);

                                            properties[nameStr]->colorListValues.push_back(color);
                                        }
                                    }
                                }
                                break;
                                }
                                propertyNames.push_back(nameStr);
                            } //isOk
                        }
                        else
                        {
                            DAVA::Logger::Error("EditorConfig::ParseConfig %s ERROR property %d unknown type %s", filePath.GetAbsolutePathname().c_str(), i, typeStr.c_str());
                        }
                    }
                    else
                    {
                        DAVA::Logger::Error("EditorConfig::ParseConfig %s ERROR property %d type or name is missing", filePath.GetAbsolutePathname().c_str(), i);
                    }
                }
                else
                {
                    DAVA::Logger::Error("EditorConfig::ParseConfig %s ERROR property %d is missing", filePath.GetAbsolutePathname().c_str(), i);
                }
            }
        }
        // else file is empty - ok, no custom properties

        parser->Release();
    }
    // else file not found - ok, no custom properties
}

const DAVA::Vector<DAVA::String>& EditorConfig::GetProjectPropertyNames() const
{
    return propertyNames;
}

const DAVA::Vector<DAVA::String>& EditorConfig::GetComboPropertyValues(const DAVA::String& nameStr) const
{
    auto itemNode = properties.find(nameStr);
    if (itemNode != properties.end())
        return itemNode->second->comboValues;
    else
    {
        return empty;
    }
}

const DAVA::Vector<DAVA::Color>& EditorConfig::GetColorPropertyValues(const DAVA::String& nameStr) const
{
    auto iter = properties.find(nameStr);
    if (iter != properties.end())
        return iter->second->colorListValues;
    else
        return emptyColors;
}

const PropertyDescription* EditorConfig::GetPropertyDescription(const DAVA::String& propertyName) const
{
    DAVA::Map<DAVA::String, PropertyDescription*>::const_iterator findIt = properties.find(propertyName);
    if (findIt != properties.end())
    {
        return findIt->second;
    }
    return NULL;
}

bool EditorConfig::HasProperty(const DAVA::String& propertyName) const
{
    return (GetPropertyDescription(propertyName) != NULL);
}

DAVA::int32 EditorConfig::GetValueTypeFromPropertyType(DAVA::int32 propertyType) const
{
    DAVA::int32 type = DAVA::VariantType::TYPE_NONE;
    switch (propertyType)
    {
    case PT_BOOL:
        type = DAVA::VariantType::TYPE_BOOLEAN;
        break;
    case PT_INT:
    case PT_COMBOBOX:
    case PT_COLOR_LIST:
        type = DAVA::VariantType::TYPE_INT32;
        break;
    case PT_STRING:
        type = DAVA::VariantType::TYPE_STRING;
        break;
    case PT_FLOAT:
        type = DAVA::VariantType::TYPE_FLOAT;
        break;
    }
    return type;
}

DAVA::int32 EditorConfig::GetPropertyValueType(const DAVA::String& propertyName) const
{
    DAVA::int32 type = DAVA::VariantType::TYPE_NONE;
    const PropertyDescription* propertyDescription = GetPropertyDescription(propertyName);
    if (propertyDescription)
    {
        type = GetValueTypeFromPropertyType(propertyDescription->type);
    }
    return type;
}

const DAVA::VariantType* EditorConfig::GetPropertyDefaultValue(const DAVA::String& propertyName) const
{
    const DAVA::VariantType* defaultValue = NULL;
    const PropertyDescription* propertyDescription = GetPropertyDescription(propertyName);
    if (propertyDescription)
    {
        defaultValue = &propertyDescription->defaultValue;
    }
    return defaultValue;
}
