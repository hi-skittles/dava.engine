#pragma once
#include "sl_CodeWriter.h"
#include "sl_Tree.h"

namespace sl
{
class GLESGenerator
{
public:
    explicit GLESGenerator(Allocator* allocator);

    bool Generate(const HLSLTree* tree, Target target, const char* entryName, std::string* code);
    const char* GetResult() const;

private:
    static const char* GetTypeName(const HLSLType& type);
    int GetFunctionArguments(HLSLFunctionCall* functionCall, HLSLExpression* expression[], int maxArguments);

    void OutputExpressionList(HLSLExpression* expression, HLSLArgument* argument = NULL);
    void OutputExpression(HLSLExpression* expression, const HLSLType* dstType = NULL);
    void OutputIdentifier(const char* name);
    void OutputArguments(HLSLArgument* argument);

    /**
     * If the statements are part of a function, then returnType can be used to specify the type
     * that a return statement is expected to produce so that correct casts will be generated.
     */
    void OutputStatements(int indent, HLSLStatement* statement, const HLSLType* returnType = NULL);

    void OutputAttribute(const HLSLType& type, const char* semantic, const char* attribType, const char* prefix);
    void OutputAttributes(HLSLFunction* entryFunction);
    void OutputDeclaration(HLSLDeclaration* declaration);
    void OutputDeclaration(const HLSLType& type, const char* name);

    void OutputSetOutAttribute(const char* semantic, const char* resultName);

    HLSLFunction* FindFunction(HLSLRoot* root, const char* name);
    HLSLStruct* FindStruct(HLSLRoot* root, const char* name);

    void Error(const char* format, ...);

    /** GLSL contains some reserved words that don't exist in HLSL. This function will
     * sanitize those names. */
    const char* GetSafeIdentifierName(const char* name) const;

    /** Generates a name of the format "base+n" where n is an integer such that the name
     * isn't used in the syntax tree. */
    bool ChooseUniqueName(const char* base, char* dst, int dstLength) const;

private:
    static const int NumReservedWords = 5;
    static const char* ReservedWord[NumReservedWords];

    CodeWriter writer;

    const HLSLTree* tree;
    const char* entryName;
    Target target;
    bool outputPosition;

    const char* outAttribPrefix;
    const char* inAttribPrefix;

    char matrixRowFunction[64];
    char clipFunction[64];
    char tex2DlodFunction[64];
    char tex2DbiasFunction[64];
    char tex3DlodFunction[64];
    char texCUBEbiasFunction[64];
    char scalarSwizzle2Function[64];
    char scalarSwizzle3Function[64];
    char scalarSwizzle4Function[64];
    char sinCosFunction[64];

    bool hasError;
    bool mrtUsed;

    char reservedWord[NumReservedWords][64];
};

} // namespace sl
