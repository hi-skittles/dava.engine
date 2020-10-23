#pragma once

#include "sl_CodeWriter.h"
#include "sl_Tree.h"

namespace sl
{
class HLSLTree;
struct HLSLFunction;
struct HLSLStruct;

class MSLGenerator
{
public:
    explicit MSLGenerator(Allocator* allocator);

    bool Generate(HLSLTree* tree, Target target, const char* entryName, std::string* code);
    const char* GetResult() const;

private:
    static const char* GetAttributeName(HLSLAttributeType attributeType);
    static const char* GetTypeName(const HLSLType& type);

    void OutputExpressionList(HLSLExpression* expression);
    void OutputExpression(HLSLExpression* expression);
    void OutputArguments(HLSLArgument* argument);
    void OutputAttributes(int indent, HLSLAttribute* attribute);
    void OutputStatements(int indent, HLSLStatement* statement);
    void OutputDeclaration(HLSLDeclaration* declaration);
    void OutputDeclaration(const HLSLType& type, const char* name, const char* semantic = NULL, const char* registerName = NULL, HLSLExpression* defaultValue = NULL);
    void OutputDeclarationType(const HLSLType& type);
    void OutputDeclarationBody(const HLSLType& type, const char* name, const char* semantic = NULL, const char* registerName = NULL, HLSLExpression* assignment = NULL);

private:
    CodeWriter writer;

    const HLSLTree* tree;
    const char* entryName;
    Target target;
    bool isInsideBuffer;

    struct
    tex_t
    {
        std::string name;
        HLSLBaseType type;
        unsigned unit;
    };
    std::vector<tex_t> texInfo;
};

} // namespace sl
