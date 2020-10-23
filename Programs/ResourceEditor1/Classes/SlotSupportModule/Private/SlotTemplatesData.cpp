#include "Classes/SlotSupportModule/Private/SlotTemplatesData.h"

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>

#include <FileSystem/FileSystem.h>
#include <FileSystem/YamlNode.h>
#include <FileSystem/YamlParser.h>
#include <Logger/Logger.h>

const SlotTemplatesData::Template* SlotTemplatesData::GetTemplate(DAVA::FastName name) const
{
    auto iter = templates.find(name);
    if (iter == templates.end())
    {
        return nullptr;
    }

    return &(iter->second);
}

DAVA::Vector<SlotTemplatesData::Template> SlotTemplatesData::GetTemplates() const
{
    DAVA::Vector<Template> result;
    result.reserve(templates.size());

    for (const auto& node : templates)
    {
        result.push_back(node.second);
    }

    return result;
}

void SlotTemplatesData::Clear()
{
    templates.clear();
}

void SlotTemplatesData::ParseConfig(const DAVA::FilePath& configPath)
{
    using namespace DAVA;

    if (GetEngineContext()->fileSystem->Exists(configPath) == false)
    {
        return;
    }

    ScopedPtr<YamlParser> parser(YamlParser::Create(configPath));
    if (!parser)
    {
        Logger::Error("Couldn't parse slot templates: %s", configPath.GetAbsolutePathname().c_str());
        return;
    }

    YamlNode* rootNode = parser->GetRootNode();
    if (rootNode == nullptr)
    {
        Logger::Error("Slot templates file %s is empty", configPath.GetAbsolutePathname().c_str());
        return;
    }

    if (rootNode->GetType() != YamlNode::eType::TYPE_ARRAY)
    {
        Logger::Error("Incorrect format of slot templates: %s", configPath.GetAbsolutePathname().c_str());
        return;
    }

    String nameKey("name");
    String sizeKey("size");
    String pivotKey("pivot");

    const DAVA::Vector<DAVA::YamlNode*>& yamlNodes = rootNode->AsVector();
    size_t propertiesCount = yamlNodes.size();
    for (YamlNode* currentNode : yamlNodes)
    {
        uint32 fieldsCount = currentNode->GetCount();

        Bitset<3> fields;
        Template t;
        for (uint32 fieldIndex = 0; fieldIndex < fieldsCount; ++fieldIndex)
        {
            const YamlNode* fieldNode = currentNode->Get(fieldIndex);
            const String& key = currentNode->GetItemKeyName(fieldIndex);
            if (key == nameKey)
            {
                t.name = FastName(fieldNode->AsString());
                fields.set(0);
            }
            else if (key == sizeKey)
            {
                if (fieldNode->GetType() == YamlNode::TYPE_ARRAY && fieldNode->GetCount() == 3)
                {
                    t.boundingBoxSize = fieldNode->AsVector3();
                    fields.set(1);
                }
                else
                {
                    Logger::Error("Incorrect format of field 'size' in template %s", t.name.c_str());
                }
            }
            else if (key == pivotKey)
            {
                if (fieldNode->GetType() == YamlNode::TYPE_ARRAY && fieldNode->GetCount() == 3)
                {
                    t.pivot = fieldNode->AsVector3();
                    fields.set(2);
                }
                else
                {
                    Logger::Error("Incorrect format of field 'size' in template %s", t.name.c_str());
                }
            }
        }

        if (fields.all())
        {
            templates.emplace(t.name, t);
        }
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(SlotTemplatesData)
{
    DAVA::ReflectionRegistrator<SlotTemplatesData>::Begin()
    .End();
}
