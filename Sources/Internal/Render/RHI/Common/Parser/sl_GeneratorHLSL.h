#pragma once
#include "sl_CodeWriter.h"
#include "sl_Tree.h"

namespace sl
{
class HLSLTree;
struct HLSLFunction;
struct HLSLStruct;

class HLSLGenerator
{
public:
    enum Mode
    {
        MODE_DX11,
        MODE_DX9
    };

    explicit HLSLGenerator(Allocator* allocator);

    bool Generate(HLSLTree* tree, Mode mode, Target target, const char* entryName, std::string* code);
    const char* GetResult() const;

private:
    static const char* GetTypeName(const HLSLType& type);
    static int GetFunctionArguments(HLSLFunctionCall* functionCall, HLSLExpression* expression[], int maxArguments);

    const char* TranslateSemantic(const char* semantic, bool output, Target target) const;

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
    Mode mode;
    const char* entryName;
    Target target;
    bool isInsideBuffer;
};

} // namespace sl
