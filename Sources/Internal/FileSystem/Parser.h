#ifndef __DAVAENGINE_PARSER_H__
#define __DAVAENGINE_PARSER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/BaseObject.h"

namespace DAVA
{
class Parser : public BaseObject
{
public:
    Parser();
    ~Parser();

    bool Load(const String& filename);
    char* Preprocess();

    bool IsDefined(const char* define);
    void AddDefine(const char* define);
    void AddDefine(const char* define, const char* value);

    int32 SkipSpaces(const char* stream);
    int32 ReadToken(const char* stream, String& token);
    int32 ReadDigit(const char* stream, String& digit);
    int32 ExpectSymbol(const char* stream, const char symbol);

private:
    struct Define
    {
        String define;
        Vector<String> argList;
        String value;
    };

    Map<String, Define> defines;
    char* textFile;
    uint32 textFileInitialSize;
};
};

#endif // __DAVAENGINE_SHADER_GRAPH_H__
