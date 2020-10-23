#include "FileSystem/YamlParser.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/YamlNode.h"
#include "Logger/Logger.h"
#include "Utils/Utils.h"

#define YAML_DECLARE_STATIC
#include "yaml/yaml.h"

namespace DAVA
{
bool YamlParser::Parse(const String& data)
{
    YamlDataHolder dataHolder;
    dataHolder.fileSize = static_cast<uint32>(data.size());
    dataHolder.data = const_cast<uint8*>(reinterpret_cast<const uint8*>(data.c_str()));
    dataHolder.dataOffset = 0;

    return Parse(&dataHolder);
}

bool YamlParser::Parse(const FilePath& pathName)
{
    RefPtr<File> yamlFile(File::Create(pathName, File::OPEN | File::READ));
    if (!yamlFile)
    {
        Logger::Error("[YamlParser::Parse] Can't Open file %s for read", pathName.GetAbsolutePathname().c_str());
        return false;
    }

    YamlDataHolder dataHolder;
    dataHolder.fileSize = static_cast<uint32>(yamlFile->GetSize());
    dataHolder.data = new uint8[dataHolder.fileSize];
    dataHolder.dataOffset = 0;
    yamlFile->Read(dataHolder.data, dataHolder.fileSize);

    bool result = Parse(&dataHolder);
    SafeDeleteArray(dataHolder.data);
    return result;
}

bool YamlParser::Parse(YamlDataHolder* dataHolder)
{
    yaml_parser_t parser;
    yaml_event_t event;

    int done = 0;

    /* Create the Parser object. */
    yaml_parser_initialize(&parser);

    yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);

    /* Set a string input. */
    //yaml_parser_set_input_string(&parser, (const unsigned char*)pathName.c_str(), pathName.length());

    yaml_parser_set_input(&parser, read_handler, dataHolder);

    String lastMapKey;
    bool isKeyPresent = false;

    /* Read the event sequence. */
    while (!done)
    {
        /* Get the next event. */
        if (!yaml_parser_parse(&parser, &event))
        {
            Logger::Error("[YamlParser::Parse] error: type: %d %s line: %d pos: %d", parser.error, parser.problem, parser.problem_mark.line, parser.problem_mark.column);
            break;
        }

        switch (event.type)
        {
        case YAML_ALIAS_EVENT:
            Logger::FrameworkDebug("[YamlParser::Parse] alias: %s", event.data.alias.anchor);
            break;

        case YAML_SCALAR_EVENT:
        {
            String scalarValue = reinterpret_cast<const char*>(event.data.scalar.value);

            if (objectStack.empty())
            {
                RefPtr<YamlNode> node = YamlNode::CreateStringNode();
                node->Set(scalarValue);
                rootObject = node;
            }
            else
            {
                RefPtr<YamlNode> topContainer = objectStack.top();
                DVASSERT(topContainer->GetType() != YamlNode::TYPE_STRING);
                if (topContainer->GetType() == YamlNode::TYPE_MAP)
                {
                    if (!isKeyPresent)
                    {
                        lastMapKey = scalarValue;
                    }
                    else
                    {
                        topContainer->Add(lastMapKey, scalarValue);
                    }
                    isKeyPresent = !isKeyPresent;
                }
                else if (topContainer->GetType() == YamlNode::TYPE_ARRAY)
                {
                    topContainer->Add(scalarValue);
                }
            }
        }
        break;

        case YAML_DOCUMENT_START_EVENT:
            //Logger::FrameworkDebug("document start:");
            break;

        case YAML_DOCUMENT_END_EVENT:
            //Logger::FrameworkDebug("document end:");
            break;

        case YAML_SEQUENCE_START_EVENT:
        {
            RefPtr<YamlNode> node = YamlNode::CreateArrayNode();
            if (objectStack.empty())
            {
                rootObject = node;
            }
            else
            {
                RefPtr<YamlNode> topContainer = objectStack.top();
                DVASSERT(topContainer->GetType() != YamlNode::TYPE_STRING);
                if (topContainer->GetType() == YamlNode::TYPE_MAP)
                {
                    DVASSERT(isKeyPresent);
                    topContainer->AddNodeToMap(lastMapKey, node);
                    isKeyPresent = false;
                }
                else if (topContainer->GetType() == YamlNode::TYPE_ARRAY)
                {
                    topContainer->AddNodeToArray(node);
                }
            }
            objectStack.push(node);
        }
        break;

        case YAML_SEQUENCE_END_EVENT:
        {
            objectStack.pop();
        }
        break;

        case YAML_MAPPING_START_EVENT:
        {
            RefPtr<YamlNode> node = YamlNode::CreateMapNode();
            if (objectStack.empty())
            {
                rootObject = node;
            }
            else
            {
                RefPtr<YamlNode> topContainer = objectStack.top();
                if (topContainer->GetType() == YamlNode::TYPE_MAP)
                {
                    DVASSERT(isKeyPresent);
                    topContainer->AddNodeToMap(lastMapKey, node);
                    isKeyPresent = false;
                }
                else if (topContainer->GetType() == YamlNode::TYPE_ARRAY)
                {
                    topContainer->AddNodeToArray(node);
                }
            }
            objectStack.push(node);
        }
        break;

        case YAML_MAPPING_END_EVENT:
        {
            objectStack.pop();
        }
        break;

        case YAML_NO_EVENT:
        case YAML_STREAM_END_EVENT:
        case YAML_STREAM_START_EVENT:
        default:
            break;
        };

        /* Are we finished? */
        done = (event.type == YAML_STREAM_END_EVENT);

        /* The application is responsible for destroying the event object. */
        yaml_event_delete(&event);
    }

    /* Destroy the Parser object. */
    yaml_parser_delete(&parser);

    DVASSERT(objectStack.size() == 0);

    return objectStack.empty();
}

YamlParser::YamlParser()
{
}

YamlParser::~YamlParser()
{
}

YamlNode* YamlParser::GetRootNode() const
{
    return rootObject.Get();
}
}
