#pragma once

#include "Render/RHI/Common/Preprocessor/PreprocessorHelpers.h"

namespace DAVA
{
class ExpressionEvaluator
{
public:
    ExpressionEvaluator();
    ~ExpressionEvaluator();

    bool Evaluate(const char* expression, float32* out);

    bool SetVariable(const char* var, float32 value);
    void RemoveVariable(const char* var);
    void ClearVariables();

    bool HasVariable(const char* name) const;

    // returns
    // true, if there was error and fills provided buffer with error message
    // false, when no error occured (err_buffer is not changed)
    bool GetLastError(char* err_buffer, uint32 err_buffer_size);

    typedef float32 (*FuncImpl)(float32 arg);

    static bool RegisterFunction(const char* name, FuncImpl impl);
    static void RegisterCommonFunctions();

private:
    struct SyntaxTreeNode;

    enum : uint32
    {
        EXPRERR_NONE = 0,
        EXPRERR_MISSING_OPERAND = 1,
        EXPRERR_UNMATCHED_PARENTHESIS = 2,
        EXPRERR_UNKNOWN_SYMBOL = 3,

        EXPRESSION_BUFFER_SIZE = 4096
    };

    void Reset();
    void PopConnectPush();
    bool EvaluateInternal(const SyntaxTreeNode* node, float32* out, uint32* err_code, uint32* err_index);

private:
    char expressionText[EXPRESSION_BUFFER_SIZE];
    Vector<SyntaxTreeNode> operatorStack;
    Vector<uint32> nodeStack;
    Vector<SyntaxTreeNode> nodeArray;
    UnorderedMap<uint32, float32> varMap;
    uint32 lastErrorCode = 0;
    uint32 lastErrorIndex = EXPRERR_NONE;

    static UnorderedMap<uint32, FuncImpl> FuncImplMap;
};
}
