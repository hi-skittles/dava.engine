#ifndef __DAVAENGINE_YAML_DOM_PARSER_H__
#define __DAVAENGINE_YAML_DOM_PARSER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class YamlNode;
/**
	\defgroup yaml Yaml configs
 */

/** 
	\ingroup yaml
	\brief this class is yaml parser and it used if you want to parse yaml file
 */
class YamlParser : public BaseObject
{
protected:
    YamlParser();
    virtual ~YamlParser();

public:
    // This method creates the parser and parses the input file.
    static RefPtr<YamlParser> Create(const FilePath& fileName)
    {
        return YamlParser::CreateAndParse(fileName);
    }

    // This method creates the parser and parses the data string.
    static RefPtr<YamlParser> CreateAndParseString(const String& data)
    {
        return YamlParser::CreateAndParse(data);
    }

    // Get the root node.
    YamlNode* GetRootNode() const;

    struct YamlDataHolder
    {
        uint32 fileSize;
        uint32 dataOffset;
        uint8* data;
    };

protected:
    template <typename T>
    static RefPtr<YamlParser> CreateAndParse(const T& data)
    {
        RefPtr<YamlParser> parser(new YamlParser());
        if (parser)
        {
            bool parseResult = parser->Parse(data);
            if (!parseResult)
            {
                return RefPtr<YamlParser>();
            }
        }
        return parser;
    }

    bool Parse(const String& fileName);
    bool Parse(const FilePath& fileName);
    bool Parse(YamlDataHolder* dataHolder);

private:
    RefPtr<YamlNode> rootObject;

    Stack<RefPtr<YamlNode>> objectStack;
};
};

#endif // __DAVAENGINE_JSON_PARSER_H__
