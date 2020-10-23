#include "Render/RHI/Common/ExpressionEvaluator.h"
#include "Render/RHI/Common/Preprocessor/PreprocessorHelpers.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
#include "Base/Hash.h"
#include "rhi_Utils.h"

namespace DAVA
{
const char OpFunctionCall = '\xFF';
const char OpEqual = OpFunctionCall - 1;
const char OpNotEqual = OpEqual - 1;
const char OpLogicalAnd = OpNotEqual - 1;
const char OpLogicalOr = OpLogicalAnd - 1;
const char OpLogicalNot = OpLogicalOr - 1;
const char OpDefined = OpLogicalNot - 1;
const char OpNotDefined = OpDefined - 1;
const char OpGreaterOrEqual = OpNotDefined - 1;
const char OpLessOrEqual = OpGreaterOrEqual - 1;

const char Operators[] = {
    '+', '-', '*', '/', '^', '!', '>', '<',
    OpEqual, OpNotEqual, OpLogicalAnd, OpLogicalOr, OpLogicalNot,
    OpDefined, OpNotDefined,
    OpGreaterOrEqual, OpLessOrEqual,
    '\0'
};

uint32 OperationPriority(char operation)
{
    uint32 ret = 0;

    switch (operation)
    {
    case '!':
        ++ret;

    case OpDefined:
    case OpNotDefined:
        ++ret;

    case OpLogicalNot:
        ++ret;

    case '^':
        ++ret;

    case '*':
    case '/':
        ++ret;

    case '+':
    case '-':
        ++ret;

    case '>':
    case '<':
    case OpEqual:
    case OpNotEqual:
    case OpGreaterOrEqual:
    case OpLessOrEqual:
        ++ret;

    case OpLogicalAnd:
        ++ret;

    case OpLogicalOr:
        ++ret;

    case OpFunctionCall:
        ++ret;
    }

    return ret;
}

static const float32 Epsilon = 0.000001f;

static const char* ExprEvalError[] =
{
  "", "one of operands is missing", "unmatched parenthesis", "unknown symbol"
};

struct ExpressionEvaluator::SyntaxTreeNode
{
    float32 operand = 0.0f;
    uint32 left_i = InvalidIndex;
    uint32 right_i = InvalidIndex;
    uint32 expr_index = 0; // for error reporting
    char operation = 0;
    FuncImpl func = nullptr;

    SyntaxTreeNode() = default;

    SyntaxTreeNode(float32 number, uint32 index)
        : operand(number)
        , expr_index(index)
    {
    }

    SyntaxTreeNode(char op, uint32 index)
        : expr_index(index)
        , operation(op)
    {
    }

    SyntaxTreeNode(char op, FuncImpl f, uint32 index)
        : expr_index(index)
        , operation(op)
        , func(f)
    {
    }
};

//------------------------------------------------------------------------------

ExpressionEvaluator::ExpressionEvaluator()
{
    memset(expressionText, 0, sizeof(expressionText));
}

//------------------------------------------------------------------------------

ExpressionEvaluator::~ExpressionEvaluator()
{
}

//------------------------------------------------------------------------------

void ExpressionEvaluator::Reset()
{
    operatorStack.clear();
    nodeStack.clear();
    nodeArray.clear();
    lastErrorCode = EXPRERR_NONE;
    lastErrorIndex = 0;
}

//------------------------------------------------------------------------------

void ExpressionEvaluator::PopConnectPush()
{
    operatorStack.back().right_i = nodeStack.back();
    nodeStack.pop_back();
    if (operatorStack.back().operation != OpDefined && operatorStack.back().operation != OpNotDefined)
    {
        operatorStack.back().left_i = nodeStack.back();
        nodeStack.pop_back();
    }

    nodeStack.push_back(uint32(nodeArray.size()));
    nodeArray.push_back(operatorStack.back());
    operatorStack.pop_back();
}

//------------------------------------------------------------------------------

bool ExpressionEvaluator::EvaluateInternal(const SyntaxTreeNode* node, float32* out, uint32* err_code, uint32* err_index)
{
    DVASSERT(out);
    *out = 0;

    if (node)
    {
        if (node->operation)
        {
            if (((node->operation == OpFunctionCall || node->operation == OpLogicalNot || node->operation == OpDefined || node->operation == OpNotDefined) && node->right_i != InvalidIndex) // for funcs only right arg makes sense
                || (node->left_i != InvalidIndex && node->right_i != InvalidIndex) // for normal ops - binary operator is assumed
                )
            {
                if (node->operation == OpDefined)
                {
                    float32 val;

                    if (EvaluateInternal((&nodeArray[0]) + node->right_i, &val, err_code, err_index))
                    {
                        *out = (val == 0.0f) ? 0.0f : 1.0f;
                    }
                    else
                    {
                        return false; // error code and index already filled
                    }
                }
                else if (node->operation == OpNotDefined)
                {
                    float32 val;

                    if (EvaluateInternal((&nodeArray[0]) + node->right_i, &val, err_code, err_index))
                    {
                        *out = (val == 0.0f) ? 0.0f : 1.0f;
                    }
                    else
                    {
                        return false; // error code and index already filled
                    }
                }
                else
                {
                    float32 x, y;

                    if ((node->left_i == InvalidIndex || EvaluateInternal((&nodeArray[0]) + node->left_i, &x, err_code, err_index))
                        && EvaluateInternal((&nodeArray[0]) + node->right_i, &y, err_code, err_index)
                        )
                    {
                        switch (node->operation)
                        {
                        case '+':
                            *out = x + y;
                            break;
                        case '-':
                            *out = x - y;
                            break;
                        case '*':
                            *out = x * y;
                            break;
                        case '/':
                            *out = x / y;
                            break;
                        case '>':
                            *out = (x > y) ? 1.0f : 0.0f;
                            break;
                        case '<':
                            *out = (x < y) ? 1.0f : 0.0f;
                            break;
                        case '^':
                            *out = std::pow(x, y);
                            break;

                        case OpGreaterOrEqual:
                            *out = (x >= y) ? 1.0f : 0.0f;
                            break;

                        case OpLessOrEqual:
                            *out = (x <= y) ? 1.0f : 0.0f;
                            break;

                        case OpEqual:
                            *out = std::abs(x - y) < Epsilon ? 1.0f : 0.0f;
                            break;

                        case OpNotEqual:
                            *out = std::abs(x - y) < Epsilon ? 0.0f : 1.0f;
                            break;

                        case OpLogicalAnd:
                            *out = (std::abs(x) > Epsilon && std::abs(y) > Epsilon) ? 1.0f : 0.0f;
                            break;

                        case OpLogicalOr:
                            *out = (std::abs(x) > Epsilon || std::abs(y) > Epsilon) ? 1.0f : 0.0f;
                            break;

                        case OpLogicalNot:
                            *out = (std::abs(y) > Epsilon) ? 0.0f : 1.0f;
                            break;

                        case OpFunctionCall:
                            *out = (node->func)(y);
                            break;
                        }
                    }
                    else
                    {
                        return false; // error code and index already filled
                    }
                }
            }
            else
            {
                // not enough operands
                *err_code = 1;
                *err_index = node->expr_index;
                return false;
            }
        }
        else
        {
            *out = node->operand;
        }
    }

    return true;
}

//------------------------------------------------------------------------------

static inline uint32 _GetOperand(const char* expression, float32* operand)
{
    uint32 ret = 0;
    const char* expr = expression;

    while (*expr && (IsValidDigitChar(*expr) || (*expr == '.')))
    {
        ++expr;
        ++ret;
    }

    if (operand)
        *operand = std::stof(expression);

    return ret;
}

//------------------------------------------------------------------------------

static inline uint32 _GetVariable(const char* expression)
{
    uint32 ret = 0;
    const char* expr = expression;

    while (*expr && IsValidAlphaNumericChar(*expr))
    {
        ++expr;
        ++ret;
    }

    return ret;
}

//------------------------------------------------------------------------------

bool ExpressionEvaluator::Evaluate(const char* expression, float32* result)
{
    DVASSERT(result != nullptr);

    uint32 len = uint32(strlen(expression));
    DVASSERT(len > 0);
    DVASSERT(len < EXPRESSION_BUFFER_SIZE);

    memset(expressionText, 0, sizeof(expressionText));

    const char* s = expression;
    char* d = expressionText;
    bool ignore_closing_brace = false;

    while (*s && *s != '\n' && *s != '\r')
    {
        if (*s == ')' && ignore_closing_brace)
        {
            ++s;
            ignore_closing_brace = false;
        }
        else if (*s == '=' && *(s + 1) == '=')
        {
            *d++ = OpEqual;
            s += 2;
        }
        else if (*s == '>' && *(s + 1) == '=')
        {
            *d++ = OpGreaterOrEqual;
            s += 2;
        }
        else if (*s == '<' && *(s + 1) == '=')
        {
            *d++ = OpLessOrEqual;
            s += 2;
        }
        else if (*s == '!' && *(s + 1) == '=')
        {
            *d++ = OpNotEqual;
            s += 2;
        }
        else if (*s == '&' && *(s + 1) == '&')
        {
            *d++ = OpLogicalAnd;
            s += 2;
        }
        else if (*s == '|' && *(s + 1) == '|')
        {
            *d++ = OpLogicalOr;
            s += 2;
        }
        else if (strnicmp(s, "!defined", 8) == 0)
        {
            *d++ = OpNotDefined;

            const char* t = s + 8;
            while (*t == ' ' || *t == '\t')
                ++t;
            if (*t == '(')
            {
                DVASSERT(!ignore_closing_brace);
                ignore_closing_brace = true;
            }

            s += 8 + 1;
        }
        else if (strnicmp(s, "defined", 7) == 0)
        {
            *d++ = OpDefined;

            const char* t = s + 7;
            while (*t == ' ' || *t == '\t')
                ++t;
            if (*t == '(')
            {
                DVASSERT(!ignore_closing_brace);
                ignore_closing_brace = true;
            }

            s += 7 + 1;
        }
        else if (*s == '!')
        {
            // replace '!' with op-code only for brace-enclosed sub-expressions;
            // regular 'logical not' case handled during building expr.tree
            const char* ns1 = s + 1;
            while (*ns1 && (*ns1 == ' ' || *ns1 == '\t'))
                ++ns1;

            if (*ns1 == '(')
            {
                *d++ = OpLogicalNot;
                s += 1;
            }
            else
            {
                *d++ = *s++;
            }
        }
        else
        {
            *d++ = *s++;
        }
    }
    *d = '\0';

    Reset();

    // build expr.tree

    const char* expr = expressionText; // expression;
    char var[EXPRESSION_BUFFER_SIZE] = {};
    bool last_token_operand = false;
    bool negate_operand_value = false;
    bool invert_operand_value = false;

    while (*expr)
    {
        uint32 offset = 0;

        // skip spaces
        if (IsSpaceChar(*expr))
        {
            while (IsSpaceChar(*(expr + offset)))
                ++offset;
        }
        // process operands
        else if (IsValidDigitChar(*expr))
        {
            SyntaxTreeNode node;
            offset = _GetOperand(expr, &node.operand);
            node.expr_index = uint32(expr - expressionText);

            if (negate_operand_value)
                node.operand = -node.operand;
            if (invert_operand_value)
                node.operand = (fabs(node.operand) > Epsilon) ? 0.0f : 1.0f;

            nodeStack.push_back(uint32(nodeArray.size()));
            nodeArray.push_back(node);

            last_token_operand = true;
            negate_operand_value = false;
            invert_operand_value = false;
        }
        // process variables/functions
        else if (IsValidAlphaChar(*expr))
        {
            offset = _GetVariable(expr);

            DVASSERT(offset < countof(var) - 1);
            strncpy(var, expr, offset);
            var[offset] = '\0';
            uint32 vhash = HashValue_N(var, offset);
            std::unordered_map<uint32_t, FuncImpl>::iterator func = FuncImplMap.find(vhash);

            if (func != FuncImplMap.end())
            {
                operatorStack.push_back(SyntaxTreeNode(OpFunctionCall, func->second, uint32(expr - expressionText)));
                last_token_operand = false;
            }
            else
            {
                if (varMap.find(vhash) != varMap.end())
                {
                    float32 value = varMap[vhash];
                    if (negate_operand_value)
                        value = -value;
                    if (invert_operand_value)
                        value = (fabs(value) > Epsilon) ? 0.0f : 1.0f;

                    if (operatorStack.size() && operatorStack.back().operation == OpDefined)
                        value = 1.0f;
                    if (operatorStack.size() && operatorStack.back().operation == OpNotDefined)
                        value = 0.0f;

                    nodeStack.push_back(uint32(nodeArray.size()));
                    nodeArray.push_back(SyntaxTreeNode(value, uint32(expr - expressionText)));

                    last_token_operand = true;
                    negate_operand_value = false;
                    invert_operand_value = false;
                }
                else
                {
                    if (operatorStack.size() && (operatorStack.back().operation == OpDefined || operatorStack.back().operation == OpNotDefined))
                    {
                        nodeStack.push_back(uint32(nodeArray.size()));
                        nodeArray.push_back(SyntaxTreeNode((operatorStack.back().operation == OpDefined) ? 0.0f : 1.0f, InvalidIndex));

                        last_token_operand = true;
                        negate_operand_value = false;
                        invert_operand_value = false;
                    }
                    else
                    {
                        lastErrorCode = EXPRERR_UNKNOWN_SYMBOL;
                        lastErrorIndex = uint32(expr - expressionText);
                        return false;
                    }
                }
            }
        }
        else if (*expr == OpLogicalNot)
        {
            operatorStack.push_back(SyntaxTreeNode(OpLogicalNot, uint32(expr - expressionText)));
            last_token_operand = false;
            offset = 1;
        }
        // process operators
        else if (strchr(Operators, *expr))
        {
            if (*expr == '-' && !last_token_operand)
            {
                negate_operand_value = true;

                ++expr;
                continue;
            }

            if (*expr == '!' && !last_token_operand)
            {
                invert_operand_value = true;

                ++expr;
                continue;
            }

            if (operatorStack.size() == 0
                || OperationPriority(operatorStack.back().operation) < OperationPriority(*expr)
                )
            {
                operatorStack.push_back(SyntaxTreeNode(*expr, uint32(expr - expressionText)));
            }
            else
            {
                // we need to clear stack from higher priority operators
                // and from the same-priority ( we calculate left part first )
                while (operatorStack.size() != 0
                       && OperationPriority(operatorStack.back().operation) >= OperationPriority(*expr)
                       )
                {
                    if (nodeStack.size() < 2 && operatorStack.back().operation != OpDefined && operatorStack.back().operation != OpNotDefined)
                    {
                        lastErrorCode = EXPRERR_MISSING_OPERAND;
                        lastErrorIndex = operatorStack.back().expr_index;
                        return false;
                    }
                    PopConnectPush();
                }

                operatorStack.push_back(SyntaxTreeNode(*expr, uint32(expr - expressionText)));
            }

            last_token_operand = false;

            offset = 1;
        }
        // process parenthesis
        else if (*expr == '(')
        {
            operatorStack.push_back(SyntaxTreeNode(*expr, uint32(expr - expressionText)));
            offset = 1;

            last_token_operand = false;
        }
        else if (*expr == ')')
        {
            // find open parentesis
            while ((operatorStack.size() != 0) && (operatorStack.back().operation != '('))
            {
                if (nodeStack.size() < 2)
                {
                    lastErrorCode = EXPRERR_MISSING_OPERAND;
                    lastErrorIndex = operatorStack.back().expr_index;
                    return false;
                }
                PopConnectPush();
            }

            // check that stack  didn't run out
            if (operatorStack.size() == 0)
            {
                lastErrorCode = EXPRERR_UNMATCHED_PARENTHESIS;
                lastErrorIndex = uint32(expr - expressionText);
                return false;
            }

            operatorStack.pop_back();

            // check if it was logical-NOT in form !(some-expression)
            if ((operatorStack.size() != 0) && (operatorStack.back().operation == OpLogicalNot))
            {
                operatorStack.back().right_i = nodeStack.back();
                nodeStack.pop_back();

                nodeStack.push_back(uint32(nodeArray.size()));
                nodeArray.push_back(operatorStack.back());

                operatorStack.pop_back();
            }

            // check if it was function call
            if ((operatorStack.size() != 0) && (operatorStack.back().operation == OpFunctionCall))
            {
                operatorStack.back().right_i = nodeStack.back();
                nodeStack.pop_back();

                nodeStack.push_back(uint32(nodeArray.size()));
                nodeArray.push_back(operatorStack.back());

                operatorStack.pop_back();
            }

            last_token_operand = false;

            offset = 1;
        }
        else
        {
            offset = 1; // found nothing, increment and try again
        }

        expr += offset;
    }

    while (operatorStack.size() != 0)
    {
        if (operatorStack.back().operation == '(')
        {
            lastErrorCode = EXPRERR_UNMATCHED_PARENTHESIS;
            lastErrorIndex = operatorStack.back().expr_index;
            return false;
        }
        else if (nodeStack.size() < 2 && operatorStack.back().operation != OpDefined && operatorStack.back().operation != OpNotDefined)
        {
            lastErrorCode = EXPRERR_MISSING_OPERAND;
            lastErrorIndex = operatorStack.back().expr_index;
            return false;
        }

        PopConnectPush();
    }

    return EvaluateInternal((&nodeArray[0]) + nodeStack.back(), result, &lastErrorCode, &lastErrorIndex);
}

bool ExpressionEvaluator::SetVariable(const char* var, float32 value)
{
    bool success = false;
    uint32 var_id = HashValue_N(var, uint32(strlen(var)));

    varMap[var_id] = value;

    return success;
}

void ExpressionEvaluator::RemoveVariable(const char* var)
{
    uint32 var_id = HashValue_N(var, uint32(strlen(var)));

    varMap.erase(var_id);
}

bool ExpressionEvaluator::HasVariable(const char* name) const
{
    bool success = false;
    uint32 var_id = HashValue_N(name, uint32(strlen(name)));

    return varMap.find(var_id) != varMap.end();
}

void ExpressionEvaluator::ClearVariables()
{
    varMap.clear();
}

bool ExpressionEvaluator::GetLastError(char* err_buffer, uint32 err_buffer_size)
{
    bool ret = false;

    if (lastErrorCode)
    {
        uint32 len = static_cast<uint32>(strlen(expressionText));
        char buf[2048] = {};

        DVASSERT(len + 1 < countof(buf));
        ::memset(buf, ' ', len);
        buf[lastErrorIndex] = '^';

        Snprintf(err_buffer, err_buffer_size, "%s\n%s\n%s\n", ExprEvalError[lastErrorCode], expressionText, buf);
        ret = true;
    }

    return ret;
}

bool ExpressionEvaluator::RegisterFunction(const char* name, FuncImpl impl)
{
    bool success = false;
    uint32 func_id = HashValue_N(name, uint32(strlen(name)));

    if (FuncImplMap.find(func_id) == FuncImplMap.end())
    {
        FuncImplMap[func_id] = impl;
        success = true;
    }

    return success;
}

static float32 EV_Sin(float32 x)
{
    return std::sin(x);
}
static float32 EV_Cos(float32 x)
{
    return std::cos(x);
}
static float32 EV_Abs(float32 x)
{
    return std::abs(x);
}
void ExpressionEvaluator::RegisterCommonFunctions()
{
    ExpressionEvaluator::RegisterFunction("sin", &EV_Sin);
    ExpressionEvaluator::RegisterFunction("cos", &EV_Cos);
    ExpressionEvaluator::RegisterFunction("abs", &EV_Abs);
}

UnorderedMap<uint32, ExpressionEvaluator::FuncImpl> ExpressionEvaluator::FuncImplMap;
}