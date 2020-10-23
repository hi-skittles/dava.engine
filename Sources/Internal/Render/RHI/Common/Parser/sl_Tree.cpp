#include "sl_Tree.h"

#include "Logger/Logger.h"

using DAVA::Logger;

namespace sl
{
HLSLTree::HLSLTree(Allocator* allocator)
    :
    m_allocator(allocator)
    , m_stringPool(allocator)
{
    m_firstPage = m_allocator->New<NodePage>();
    m_firstPage->next = NULL;

    m_currentPage = m_firstPage;
    m_currentPageOffset = 0;

    m_root = AddNode<HLSLRoot>(NULL, 1);
}

HLSLTree::~HLSLTree()
{
    NodePage* page = m_firstPage;
    while (page != NULL)
    {
        NodePage* next = page->next;
        m_allocator->Delete(page);
        page = next;
    }
}

void HLSLTree::AllocatePage()
{
    NodePage* newPage = m_allocator->New<NodePage>();
    newPage->next = NULL;
    m_currentPage->next = newPage;
    m_currentPageOffset = 0;
    m_currentPage = newPage;
}

const char* HLSLTree::AddString(const char* string)
{
    return m_stringPool.AddString(string);
}

bool HLSLTree::GetContainsString(const char* string) const
{
    return m_stringPool.GetContainsString(string);
}

HLSLRoot* HLSLTree::GetRoot() const
{
    return m_root;
}

void* HLSLTree::AllocateMemory(size_t size)
{
    if (m_currentPageOffset + size > s_nodePageSize)
    {
        AllocatePage();
    }
    void* buffer = m_currentPage->buffer + m_currentPageOffset;
    m_currentPageOffset += size;
    return buffer;
}

// @@ This doesn't do any parameter matching. Simply returns the first function with that name.
HLSLFunction* HLSLTree::FindFunction(const char* name, HLSLStatement** parent)
{
    HLSLStatement* statement = m_root->statement;
    HLSLStatement* pstatement = NULL;

    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Function)
        {
            HLSLFunction* function = static_cast<HLSLFunction*>(statement);
            if (String_Equal(name, function->name))
            {
                if (parent)
                    *parent = pstatement;

                return function;
            }
        }

        pstatement = statement;
        statement = statement->nextStatement;
    }

    return NULL;
}

unsigned HLSLTree::FindFunctionCall(const char* name, Array<HLSLFunctionCall*>* call)
{
    class
    Visitor
    : public HLSLTreeVisitor
    {
    public:
        const char* name;
        Array<HLSLFunctionCall*>* call;

        Visitor(const char* n, Array<HLSLFunctionCall*>* c)
            : name(n)
            , call(c)
        {
        }
        virtual void VisitFunctionCall(HLSLFunctionCall* node)
        {
            if (String_Equal(node->function->name, name))
                call->PushBack(node);
        }
    };

    Visitor visitor(name, call);

    call->Resize(0);
    visitor.VisitRoot(GetRoot());

    return call->GetSize();
}

HLSLDeclaration* HLSLTree::FindGlobalDeclaration(const char* name) const
{
    HLSLStatement* statement = m_root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Declaration)
        {
            HLSLDeclaration* declaration = static_cast<HLSLDeclaration*>(statement);
            if (String_Equal(name, declaration->name))
            {
                return declaration;
            }
        }
        else if (statement->nodeType == HLSLNodeType_Buffer)
        {
            HLSLBuffer* buffer = static_cast<HLSLBuffer*>(statement);

            HLSLDeclaration* field = buffer->field;
            while (field != NULL)
            {
                DVASSERT(field->nodeType == HLSLNodeType_Declaration);
                if (String_Equal(name, field->name))
                {
                    return field;
                }
                field = static_cast<HLSLDeclaration*>(field->nextStatement);
            }
        }

        statement = statement->nextStatement;
    }

    return NULL;
}

HLSLStruct* HLSLTree::FindGlobalStruct(const char* name) const
{
    HLSLStatement* statement = m_root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Struct)
        {
            HLSLStruct* declaration = static_cast<HLSLStruct*>(statement);
            if (String_Equal(name, declaration->name))
            {
                return declaration;
            }
        }

        statement = statement->nextStatement;
    }

    return NULL;
}

HLSLTechnique* HLSLTree::FindTechnique(const char* name)
{
    HLSLStatement* statement = m_root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Technique)
        {
            HLSLTechnique* technique = static_cast<HLSLTechnique*>(statement);
            if (String_Equal(name, technique->name))
            {
                return technique;
            }
        }

        statement = statement->nextStatement;
    }

    return NULL;
}

HLSLPipeline* HLSLTree::FindFirstPipeline()
{
    return FindNextPipeline(NULL);
}

HLSLPipeline* HLSLTree::FindNextPipeline(HLSLPipeline* current)
{
    HLSLStatement* statement = current ? current : m_root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Pipeline)
        {
            return static_cast<HLSLPipeline*>(statement);
        }

        statement = statement->nextStatement;
    }

    return NULL;
}

HLSLPipeline* HLSLTree::FindPipeline(const char* name)
{
    HLSLStatement* statement = m_root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Pipeline)
        {
            HLSLPipeline* pipeline = static_cast<HLSLPipeline*>(statement);
            if (String_Equal(name, pipeline->name))
            {
                return pipeline;
            }
        }

        statement = statement->nextStatement;
    }

    return NULL;
}

HLSLBuffer* HLSLTree::FindBuffer(const char* name) const
{
    HLSLStatement* statement = m_root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Buffer)
        {
            HLSLBuffer* buffer = static_cast<HLSLBuffer*>(statement);
            if (String_Equal(name, buffer->name))
            {
                return buffer;
            }
        }

        statement = statement->nextStatement;
    }

    return NULL;
}

bool HLSLTree::GetExpressionValue(HLSLExpression* expression, int& value)
{
    DVASSERT(expression != NULL);

    // Expression must be constant.
    if ((expression->expressionType.flags & HLSLTypeFlag_Const) == 0)
    {
        return false;
    }

    // We are expecting an integer scalar. @@ Add support for type conversion from other scalar types.
    if (expression->expressionType.baseType != HLSLBaseType_Int)
    {
        return false;
    }

    if (expression->expressionType.array)
    {
        return false;
    }

    if (expression->nodeType == HLSLNodeType_BinaryExpression)
    {
        HLSLBinaryExpression* binaryExpression = static_cast<HLSLBinaryExpression*>(expression);

        int value1, value2;
        if (!GetExpressionValue(binaryExpression->expression1, value1) ||
            !GetExpressionValue(binaryExpression->expression2, value2))
        {
            return false;
        }

        switch (binaryExpression->binaryOp)
        {
        case HLSLBinaryOp_And:
            value = value1 && value2;
            return true;
        case HLSLBinaryOp_Or:
            value = value1 || value2;
            return true;
        case HLSLBinaryOp_Add:
            value = value1 + value2;
            return true;
        case HLSLBinaryOp_Sub:
            value = value1 - value2;
            return true;
        case HLSLBinaryOp_Mul:
            value = value1 * value2;
            return true;
        case HLSLBinaryOp_Div:
            value = value1 / value2;
            return true;
        case HLSLBinaryOp_Less:
            value = value1 < value2;
            return true;
        case HLSLBinaryOp_Greater:
            value = value1 > value2;
            return true;
        case HLSLBinaryOp_LessEqual:
            value = value1 <= value2;
            return true;
        case HLSLBinaryOp_GreaterEqual:
            value = value1 >= value2;
            return true;
        case HLSLBinaryOp_Equal:
            value = value1 == value2;
            return true;
        case HLSLBinaryOp_NotEqual:
            value = value1 != value2;
            return true;
        case HLSLBinaryOp_BitAnd:
            value = value1 & value2;
            return true;
        case HLSLBinaryOp_BitOr:
            value = value1 | value2;
            return true;
        case HLSLBinaryOp_BitXor:
            value = value1 ^ value2;
            return true;
        default:
            break; // to shut up goddamn warning
        }
    }
    else if (expression->nodeType == HLSLNodeType_UnaryExpression)
    {
        HLSLUnaryExpression* unaryExpression = static_cast<HLSLUnaryExpression*>(expression);

        if (!GetExpressionValue(unaryExpression->expression, value))
        {
            return false;
        }

        switch (unaryExpression->unaryOp)
        {
        case HLSLUnaryOp_Negative:
            value = -value;
            return true;
        case HLSLUnaryOp_Positive:
            // nop.
            return true;
        case HLSLUnaryOp_Not:
            value = !value;
            return true;
        case HLSLUnaryOp_BitNot:
            value = ~value;
            return true;
        default:
            break; // to shut up goddamn warning
        }
    }
    else if (expression->nodeType == HLSLNodeType_IdentifierExpression)
    {
        HLSLIdentifierExpression* identifier = static_cast<HLSLIdentifierExpression*>(expression);

        HLSLDeclaration* declaration = FindGlobalDeclaration(identifier->name);
        if (declaration == NULL)
        {
            return false;
        }
        if ((declaration->type.flags & HLSLTypeFlag_Const) == 0)
        {
            return false;
        }

        return GetExpressionValue(declaration->assignment, value);
    }
    else if (expression->nodeType == HLSLNodeType_LiteralExpression)
    {
        HLSLLiteralExpression* literal = static_cast<HLSLLiteralExpression*>(expression);
        value = literal->iValue;
        return true;
    }

    return false;
}

/*
bool HLSLTree::GetExpressionValue(HLSLExpression * expression, float & value)
{
    ASSERT (expression != NULL);

    // Expression must be constant.
    if ((expression->expressionType.flags & HLSLTypeFlag_Const) == 0) 
    {
        return false;
    }

    // We are expecting an integer scalar. @@ Add support for type conversion from other scalar types.
    if (expression->expressionType.baseType != HLSLBaseType_Float) 
    {
        return false;
    }

    if (expression->expressionType.array) 
    {
        return false;
    }

    if (expression->nodeType == HLSLNodeType_BinaryExpression) 
    {
        HLSLBinaryExpression * binaryExpression = (HLSLBinaryExpression *)expression;

        int value1, value2;
        if (!GetExpressionValue(binaryExpression->expression1, value1) ||
            !GetExpressionValue(binaryExpression->expression2, value2))
        {
            return false;
        }

        switch(binaryExpression->binaryOp)
        {
            case HLSLBinaryOp_Add:
                value = value1 + value2;
                return true;
            case HLSLBinaryOp_Sub:
                value = value1 - value2;
                return true;
            case HLSLBinaryOp_Mul:
                value = value1 * value2;
                return true;
            case HLSLBinaryOp_Div:
                value = value1 / value2;
                return true;
        }
    }
    else if (expression->nodeType == HLSLNodeType_UnaryExpression) 
    {
        HLSLUnaryExpression * unaryExpression = (HLSLUnaryExpression *)expression;

        if (!GetExpressionValue(unaryExpression->expression, value))
        {
            return false;
        }

        switch(unaryExpression->unaryOp)
        {
            case HLSLUnaryOp_Negative:
                value = -value;
                return true;
            case HLSLUnaryOp_Positive:
                // nop.
                return true;
        }
    }
    else if (expression->nodeType == HLSLNodeType_IdentifierExpression)
    {
        HLSLIdentifierExpression * identifier = (HLSLIdentifierExpression *)expression;

        HLSLDeclaration * declaration = FindGlobalDeclaration(identifier->name);
        if (declaration == NULL) 
        {
            return false;
        }
        if ((declaration->type.flags & HLSLTypeFlag_Const) == 0)
        {
            return false;
        }

        return GetExpressionValue(declaration->assignment, value);
    }
    else if (expression->nodeType == HLSLNodeType_LiteralExpression)
    {
        HLSLLiteralExpression * literal = (HLSLLiteralExpression *)expression;
        value = literal->iValue;
        return true;
    }

    return false;
}
*/

void HLSLTreeVisitor::VisitType(HLSLType& type)
{
}

void HLSLTreeVisitor::VisitRoot(HLSLRoot* root)
{
    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        VisitTopLevelStatement(statement);
        statement = statement->nextStatement;
    }
}

void HLSLTreeVisitor::VisitTopLevelStatement(HLSLStatement* node)
{
    if (node->nodeType == HLSLNodeType_Declaration)
    {
        VisitDeclaration(static_cast<HLSLDeclaration*>(node));
    }
    else if (node->nodeType == HLSLNodeType_Struct)
    {
        VisitStruct(static_cast<HLSLStruct*>(node));
    }
    else if (node->nodeType == HLSLNodeType_Buffer)
    {
        VisitBuffer(static_cast<HLSLBuffer*>(node));
    }
    else if (node->nodeType == HLSLNodeType_Function)
    {
        VisitFunction(static_cast<HLSLFunction*>(node));
    }
    else if (node->nodeType == HLSLNodeType_Technique)
    {
        VisitTechnique(static_cast<HLSLTechnique*>(node));
    }
    else
    {
        DVASSERT(0);
    }
}

void HLSLTreeVisitor::VisitStatements(HLSLStatement* statement)
{
    while (statement != NULL)
    {
        VisitStatement(statement);
        statement = statement->nextStatement;
    }
}

void HLSLTreeVisitor::VisitStatement(HLSLStatement* node)
{
    // Function statements
    if (node->nodeType == HLSLNodeType_Declaration)
    {
        VisitDeclaration(static_cast<HLSLDeclaration*>(node));
    }
    else if (node->nodeType == HLSLNodeType_ExpressionStatement)
    {
        VisitExpressionStatement(static_cast<HLSLExpressionStatement*>(node));
    }
    else if (node->nodeType == HLSLNodeType_ReturnStatement)
    {
        VisitReturnStatement(static_cast<HLSLReturnStatement*>(node));
    }
    else if (node->nodeType == HLSLNodeType_DiscardStatement)
    {
        VisitDiscardStatement(static_cast<HLSLDiscardStatement*>(node));
    }
    else if (node->nodeType == HLSLNodeType_BreakStatement)
    {
        VisitBreakStatement(static_cast<HLSLBreakStatement*>(node));
    }
    else if (node->nodeType == HLSLNodeType_ContinueStatement)
    {
        VisitContinueStatement(static_cast<HLSLContinueStatement*>(node));
    }
    else if (node->nodeType == HLSLNodeType_IfStatement)
    {
        VisitIfStatement(static_cast<HLSLIfStatement*>(node));
    }
    else if (node->nodeType == HLSLNodeType_ForStatement)
    {
        VisitForStatement(static_cast<HLSLForStatement*>(node));
    }
    else if (node->nodeType == HLSLNodeType_BlockStatement)
    {
        VisitBlockStatement(static_cast<HLSLBlockStatement*>(node));
    }
    else
    {
        DVASSERT(0);
    }
}

void HLSLTreeVisitor::VisitDeclaration(HLSLDeclaration* node)
{
    VisitType(node->type);
    /*do {
        VisitExpression(node->assignment);
        node = node->nextDeclaration;
    } while (node);*/
    if (node->assignment != NULL)
    {
        VisitExpression(node->assignment);
    }
    if (node->nextDeclaration != NULL)
    {
        VisitDeclaration(node->nextDeclaration);
    }
}

void HLSLTreeVisitor::VisitStruct(HLSLStruct* node)
{
    HLSLStructField* field = node->field;
    while (field != NULL)
    {
        VisitStructField(field);
        field = field->nextField;
    }
}

void HLSLTreeVisitor::VisitStructField(HLSLStructField* node)
{
    VisitType(node->type);
}

void HLSLTreeVisitor::VisitBuffer(HLSLBuffer* node)
{
    HLSLDeclaration* field = node->field;
    while (field != NULL)
    {
        DVASSERT(field->nodeType == HLSLNodeType_Declaration);
        VisitDeclaration(field);
        DVASSERT(field->nextDeclaration == NULL);
        field = static_cast<HLSLDeclaration*>(field->nextStatement);
    }
}

/*void HLSLTreeVisitor::VisitBufferField(HLSLBufferField * node)
{
    VisitType(node->type);
}*/

void HLSLTreeVisitor::VisitFunction(HLSLFunction* node)
{
    VisitType(node->returnType);

    HLSLArgument* argument = node->argument;
    while (argument != NULL)
    {
        VisitArgument(argument);
        argument = argument->nextArgument;
    }

    VisitStatements(node->statement);
}

void HLSLTreeVisitor::VisitArgument(HLSLArgument* node)
{
    VisitType(node->type);
    if (node->defaultValue != NULL)
    {
        VisitExpression(node->defaultValue);
    }
}

void HLSLTreeVisitor::VisitExpressionStatement(HLSLExpressionStatement* node)
{
    VisitExpression(node->expression);
}

void HLSLTreeVisitor::VisitExpression(HLSLExpression* node)
{
    VisitType(node->expressionType);

    if (node->nodeType == HLSLNodeType_UnaryExpression)
    {
        VisitUnaryExpression(static_cast<HLSLUnaryExpression*>(node));
    }
    else if (node->nodeType == HLSLNodeType_BinaryExpression)
    {
        VisitBinaryExpression(static_cast<HLSLBinaryExpression*>(node));
    }
    else if (node->nodeType == HLSLNodeType_ConditionalExpression)
    {
        VisitConditionalExpression(static_cast<HLSLConditionalExpression*>(node));
    }
    else if (node->nodeType == HLSLNodeType_CastingExpression)
    {
        VisitCastingExpression(static_cast<HLSLCastingExpression*>(node));
    }
    else if (node->nodeType == HLSLNodeType_LiteralExpression)
    {
        VisitLiteralExpression(static_cast<HLSLLiteralExpression*>(node));
    }
    else if (node->nodeType == HLSLNodeType_IdentifierExpression)
    {
        VisitIdentifierExpression(static_cast<HLSLIdentifierExpression*>(node));
    }
    else if (node->nodeType == HLSLNodeType_ConstructorExpression)
    {
        VisitConstructorExpression(static_cast<HLSLConstructorExpression*>(node));
    }
    else if (node->nodeType == HLSLNodeType_MemberAccess)
    {
        VisitMemberAccess(static_cast<HLSLMemberAccess*>(node));
    }
    else if (node->nodeType == HLSLNodeType_ArrayAccess)
    {
        VisitArrayAccess(static_cast<HLSLArrayAccess*>(node));
    }
    else if (node->nodeType == HLSLNodeType_FunctionCall)
    {
        VisitFunctionCall(static_cast<HLSLFunctionCall*>(node));
    }
    else
    {
        DVASSERT(0);
    }
}

void HLSLTreeVisitor::VisitReturnStatement(HLSLReturnStatement* node)
{
    VisitExpression(node->expression);
}

void HLSLTreeVisitor::VisitDiscardStatement(HLSLDiscardStatement* node)
{
}
void HLSLTreeVisitor::VisitBreakStatement(HLSLBreakStatement* node)
{
}
void HLSLTreeVisitor::VisitContinueStatement(HLSLContinueStatement* node)
{
}

void HLSLTreeVisitor::VisitIfStatement(HLSLIfStatement* node)
{
    VisitExpression(node->condition);
    VisitStatements(node->statement);
    if (node->elseStatement)
    {
        VisitStatements(node->elseStatement);
    }
}

void HLSLTreeVisitor::VisitForStatement(HLSLForStatement* node)
{
    if (node->initialization)
    {
        VisitDeclaration(node->initialization);
    }
    if (node->condition)
    {
        VisitExpression(node->condition);
    }
    if (node->increment)
    {
        VisitExpression(node->increment);
    }
    VisitStatements(node->statement);
}

void HLSLTreeVisitor::VisitBlockStatement(HLSLBlockStatement* node)
{
    VisitStatements(node->statement);
}

void HLSLTreeVisitor::VisitUnaryExpression(HLSLUnaryExpression* node)
{
    VisitExpression(node->expression);
}

void HLSLTreeVisitor::VisitBinaryExpression(HLSLBinaryExpression* node)
{
    VisitExpression(node->expression1);
    VisitExpression(node->expression2);
}

void HLSLTreeVisitor::VisitConditionalExpression(HLSLConditionalExpression* node)
{
    VisitExpression(node->condition);
    VisitExpression(node->falseExpression);
    VisitExpression(node->trueExpression);
}

void HLSLTreeVisitor::VisitCastingExpression(HLSLCastingExpression* node)
{
    VisitType(node->type);
    VisitExpression(node->expression);
}

void HLSLTreeVisitor::VisitLiteralExpression(HLSLLiteralExpression* node)
{
}
void HLSLTreeVisitor::VisitIdentifierExpression(HLSLIdentifierExpression* node)
{
}

void HLSLTreeVisitor::VisitConstructorExpression(HLSLConstructorExpression* node)
{
    HLSLExpression* argument = node->argument;
    while (argument != NULL)
    {
        VisitExpression(argument);
        argument = argument->nextExpression;
    }
}

void HLSLTreeVisitor::VisitMemberAccess(HLSLMemberAccess* node)
{
    VisitExpression(node->object);
}

void HLSLTreeVisitor::VisitArrayAccess(HLSLArrayAccess* node)
{
    VisitExpression(node->array);
    VisitExpression(node->index);
}

void HLSLTreeVisitor::VisitFunctionCall(HLSLFunctionCall* node)
{
    HLSLExpression* argument = node->argument;
    while (argument != NULL)
    {
        VisitExpression(argument);
        argument = argument->nextExpression;
    }
}

void HLSLTreeVisitor::VisitStateAssignment(HLSLStateAssignment* node)
{
}

void HLSLTreeVisitor::VisitSamplerState(HLSLSamplerState* node)
{
    HLSLStateAssignment* stateAssignment = node->stateAssignments;
    while (stateAssignment != NULL)
    {
        VisitStateAssignment(stateAssignment);
        stateAssignment = stateAssignment->nextStateAssignment;
    }
}

void HLSLTreeVisitor::VisitPass(HLSLPass* node)
{
    HLSLStateAssignment* stateAssignment = node->stateAssignments;
    while (stateAssignment != NULL)
    {
        VisitStateAssignment(stateAssignment);
        stateAssignment = stateAssignment->nextStateAssignment;
    }
}

void HLSLTreeVisitor::VisitTechnique(HLSLTechnique* node)
{
    HLSLPass* pass = node->passes;
    while (pass != NULL)
    {
        VisitPass(pass);
        pass = pass->nextPass;
    }
}

void HLSLTreeVisitor::VisitFunctions(HLSLRoot* root)
{
    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Function)
        {
            VisitFunction(static_cast<HLSLFunction*>(statement));
        }

        statement = statement->nextStatement;
    }
}

void HLSLTreeVisitor::VisitParameters(HLSLRoot* root)
{
    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Declaration)
        {
            VisitDeclaration(static_cast<HLSLDeclaration*>(statement));
        }

        statement = statement->nextStatement;
    }
}

class ResetHiddenFlagVisitor : public HLSLTreeVisitor
{
public:
    virtual void VisitTopLevelStatement(HLSLStatement* statement)
    {
        statement->hidden = true;

        if (statement->nodeType == HLSLNodeType_Buffer)
        {
            VisitBuffer(static_cast<HLSLBuffer*>(statement));
        }
    }

    // Hide buffer fields.
    virtual void VisitDeclaration(HLSLDeclaration* node)
    {
        node->hidden = true;
    }
};

class MarkVisibleStatementsVisitor : public HLSLTreeVisitor
{
public:
    HLSLTree* tree;
    MarkVisibleStatementsVisitor(HLSLTree* tree)
        : tree(tree)
    {
    }

    virtual void VisitFunction(HLSLFunction* node)
    {
        node->hidden = false;
        HLSLTreeVisitor::VisitFunction(node);
    }

    virtual void VisitFunctionCall(HLSLFunctionCall* node)
    {
        HLSLTreeVisitor::VisitFunctionCall(node);

        if (node->function->hidden)
        {
            VisitFunction(const_cast<HLSLFunction*>(node->function));
        }
    }

    virtual void VisitIdentifierExpression(HLSLIdentifierExpression* node)
    {
        HLSLTreeVisitor::VisitIdentifierExpression(node);

        if (node->global)
        {
            HLSLDeclaration* declaration = tree->FindGlobalDeclaration(node->name);
            if (declaration != NULL && declaration->hidden)
            {
                declaration->hidden = false;
                VisitDeclaration(declaration);
            }
        }
    }

    virtual void VisitType(HLSLType& type)
    {
        if (type.baseType == HLSLBaseType_UserDefined)
        {
            HLSLStruct* globalStruct = tree->FindGlobalStruct(type.typeName);
            if (globalStruct != NULL)
            {
                globalStruct->hidden = false;
                VisitStruct(globalStruct);
            }
        }
    }
};

void PruneTree(HLSLTree* tree, const char* entryName0, const char* entryName1 /*=NULL*/)
{
    HLSLRoot* root = tree->GetRoot();

    // Reset all flags.
    ResetHiddenFlagVisitor reset;
    reset.VisitRoot(root);

    // Mark all the statements necessary for these entrypoints.
    HLSLFunction* entry = tree->FindFunction(entryName0);
    if (entry != NULL)
    {
        MarkVisibleStatementsVisitor mark(tree);
        mark.VisitFunction(entry);
    }

    if (entryName1 != NULL)
    {
        entry = tree->FindFunction(entryName1);
        if (entry != NULL)
        {
            MarkVisibleStatementsVisitor mark(tree);
            mark.VisitFunction(entry);
        }
    }

    // Mark buffers visible, if any of their fields is visible.
    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Buffer)
        {
            HLSLBuffer* buffer = static_cast<HLSLBuffer*>(statement);

            HLSLDeclaration* field = buffer->field;
            while (field != NULL)
            {
                DVASSERT(field->nodeType == HLSLNodeType_Declaration);
                if (!field->hidden)
                {
                    buffer->hidden = false;
                    break;
                }
                field = static_cast<HLSLDeclaration*>(field->nextStatement);
            }
        }

        statement = statement->nextStatement;
    }
}

void SortTree(HLSLTree* tree)
{
    // Stable sort so that statements are in this order:
    // structs, declarations, functions, techniques.
    // but their relative order is preserved.

    HLSLRoot* root = tree->GetRoot();

    HLSLStatement* structs = NULL;
    HLSLStatement* lastStruct = NULL;
    HLSLStatement* constDeclarations = NULL;
    HLSLStatement* lastConstDeclaration = NULL;
    HLSLStatement* declarations = NULL;
    HLSLStatement* lastDeclaration = NULL;
    HLSLStatement* functions = NULL;
    HLSLStatement* lastFunction = NULL;
    HLSLStatement* other = NULL;
    HLSLStatement* lastOther = NULL;

    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        HLSLStatement* nextStatement = statement->nextStatement;
        statement->nextStatement = NULL;

        if (statement->nodeType == HLSLNodeType_Struct)
        {
            if (structs == NULL)
                structs = statement;
            if (lastStruct != NULL)
                lastStruct->nextStatement = statement;
            lastStruct = statement;
        }
        else if (statement->nodeType == HLSLNodeType_Declaration || statement->nodeType == HLSLNodeType_Buffer)
        {
            if (statement->nodeType == HLSLNodeType_Declaration && ((static_cast<HLSLDeclaration*>(statement))->type.flags & HLSLTypeFlag_Const))
            {
                if (constDeclarations == NULL)
                    constDeclarations = statement;
                if (lastConstDeclaration != NULL)
                    lastConstDeclaration->nextStatement = statement;
                lastConstDeclaration = statement;
            }
            else
            {
                if (declarations == NULL)
                    declarations = statement;
                if (lastDeclaration != NULL)
                    lastDeclaration->nextStatement = statement;
                lastDeclaration = statement;
            }
        }
        else if (statement->nodeType == HLSLNodeType_Function)
        {
            if (functions == NULL)
                functions = statement;
            if (lastFunction != NULL)
                lastFunction->nextStatement = statement;
            lastFunction = statement;
        }
        else
        {
            if (other == NULL)
                other = statement;
            if (lastOther != NULL)
                lastOther->nextStatement = statement;
            lastOther = statement;
        }

        statement = nextStatement;
    }

    // Chain all the statements in the order that we want.
    HLSLStatement* firstStatement = structs;
    HLSLStatement* lastStatement = lastStruct;

    if (constDeclarations != NULL)
    {
        if (firstStatement == NULL)
            firstStatement = constDeclarations;
        else
            lastStatement->nextStatement = constDeclarations;
        lastStatement = lastConstDeclaration;
    }

    if (declarations != NULL)
    {
        if (firstStatement == NULL)
            firstStatement = declarations;
        else
            lastStatement->nextStatement = declarations;
        lastStatement = lastDeclaration;
    }

    if (functions != NULL)
    {
        if (firstStatement == NULL)
            firstStatement = functions;
        else
            lastStatement->nextStatement = functions;
        lastStatement = lastFunction;
    }

    if (other != NULL)
    {
        if (firstStatement == NULL)
            firstStatement = other;
        else
            lastStatement->nextStatement = other;
        lastStatement = lastOther;
    }

    root->statement = firstStatement;
}

// First and last can be the same.
void AddStatements(HLSLRoot* root, HLSLStatement* before, HLSLStatement* first, HLSLStatement* last)
{
    if (before == NULL)
    {
        last->nextStatement = root->statement;
        root->statement = first;
    }
    else
    {
        last->nextStatement = before->nextStatement;
        before->nextStatement = first;
    }
}

void AddSingleStatement(HLSLRoot* root, HLSLStatement* before, HLSLStatement* statement)
{
    AddStatements(root, before, statement, statement);
}

// @@ This is very game-specific. Should be moved to pipeline_parser or somewhere else.
void GroupParameters(HLSLTree* tree)
{
    // Sort parameters based on semantic and group them in cbuffers.

    HLSLRoot* root = tree->GetRoot();

    HLSLDeclaration* firstPerItemDeclaration = NULL;
    HLSLDeclaration* lastPerItemDeclaration = NULL;

    HLSLDeclaration* instanceDataDeclaration = NULL;

    HLSLDeclaration* firstPerPassDeclaration = NULL;
    HLSLDeclaration* lastPerPassDeclaration = NULL;

    HLSLDeclaration* firstPerItemSampler = NULL;
    HLSLDeclaration* lastPerItemSampler = NULL;

    HLSLDeclaration* firstPerPassSampler = NULL;
    HLSLDeclaration* lastPerPassSampler = NULL;

    HLSLStatement* statementBeforeBuffers = NULL;

    HLSLStatement* previousStatement = NULL;
    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        HLSLStatement* nextStatement = statement->nextStatement;

        if (statement->nodeType == HLSLNodeType_Struct) // Do not remove this, or it will mess the else clause below.
        {
            statementBeforeBuffers = statement;
        }
        else if (statement->nodeType == HLSLNodeType_Declaration)
        {
            HLSLDeclaration* declaration = static_cast<HLSLDeclaration*>(statement);

            // We insert buffers after the last const declaration.
            if ((declaration->type.flags & HLSLTypeFlag_Const) != 0)
            {
                statementBeforeBuffers = statement;
            }

            // Do not move samplers or static/const parameters.
            if ((declaration->type.flags & (HLSLTypeFlag_Static | HLSLTypeFlag_Const)) == 0)
            {
                // Unlink statement.
                statement->nextStatement = NULL;
                if (previousStatement != NULL)
                    previousStatement->nextStatement = nextStatement;
                else
                    root->statement = nextStatement;

                while (declaration != NULL)
                {
                    HLSLDeclaration* nextDeclaration = declaration->nextDeclaration;

                    if (declaration->semantic != NULL && String_EqualNoCase(declaration->semantic, "PER_INSTANCED_ITEM"))
                    {
                        DVASSERT(instanceDataDeclaration == NULL);
                        instanceDataDeclaration = declaration;
                    }
                    else
                    {
                        // Select group based on type and semantic.
                        HLSLDeclaration **first, **last;
                        if (declaration->semantic == NULL || String_EqualNoCase(declaration->semantic, "PER_ITEM") || String_EqualNoCase(declaration->semantic, "PER_MATERIAL"))
                        {
                            if (IsSamplerType(declaration->type))
                            {
                                first = &firstPerItemSampler;
                                last = &lastPerItemSampler;
                            }
                            else
                            {
                                first = &firstPerItemDeclaration;
                                last = &lastPerItemDeclaration;
                            }
                        }
                        else
                        {
                            if (IsSamplerType(declaration->type))
                            {
                                first = &firstPerPassSampler;
                                last = &lastPerPassSampler;
                            }
                            else
                            {
                                first = &firstPerPassDeclaration;
                                last = &lastPerPassDeclaration;
                            }
                        }

                        // Add declaration to new list.
                        if (*first == NULL)
                            *first = declaration;
                        else
                            (*last)->nextStatement = declaration;
                        *last = declaration;
                    }

                    // Unlink from declaration list.
                    declaration->nextDeclaration = NULL;

                    // Reset attributes.
                    declaration->registerName = NULL;
                    //declaration->semantic = NULL;         // @@ Don't do this!

                    declaration = nextDeclaration;
                }
            }
        }
        /*else
        {
            if (statementBeforeBuffers == NULL) {
                // This is the location where we will insert our buffers.
                statementBeforeBuffers = previousStatement;
            }
        }*/

        if (statement->nextStatement == nextStatement)
        {
            previousStatement = statement;
        }
        statement = nextStatement;
    }

    // Add instance data declaration at the end of the per_item buffer.
    if (instanceDataDeclaration != NULL)
    {
        if (firstPerItemDeclaration == NULL)
            firstPerItemDeclaration = instanceDataDeclaration;
        else
            lastPerItemDeclaration->nextStatement = instanceDataDeclaration;
    }

    // Add samplers.
    if (firstPerItemSampler != NULL)
    {
        AddStatements(root, statementBeforeBuffers, firstPerItemSampler, lastPerItemSampler);
        statementBeforeBuffers = lastPerItemSampler;
    }
    if (firstPerPassSampler != NULL)
    {
        AddStatements(root, statementBeforeBuffers, firstPerPassSampler, lastPerPassSampler);
        statementBeforeBuffers = lastPerPassSampler;
    }

    // @@ We are assuming per_item and per_pass buffers don't already exist. @@ We should assert on that.

    if (firstPerItemDeclaration != NULL)
    {
        // Create buffer statement.
        HLSLBuffer* perItemBuffer = tree->AddNode<HLSLBuffer>(firstPerItemDeclaration->fileName, firstPerItemDeclaration->line - 1);
        perItemBuffer->name = tree->AddString("per_item");
        perItemBuffer->registerName = tree->AddString("b0");
        perItemBuffer->field = firstPerItemDeclaration;

        // Add buffer to statements.
        AddSingleStatement(root, statementBeforeBuffers, perItemBuffer);
        statementBeforeBuffers = perItemBuffer;
    }

    if (firstPerPassDeclaration != NULL)
    {
        // Create buffer statement.
        HLSLBuffer* perPassBuffer = tree->AddNode<HLSLBuffer>(firstPerPassDeclaration->fileName, firstPerPassDeclaration->line - 1);
        perPassBuffer->name = tree->AddString("per_pass");
        perPassBuffer->registerName = tree->AddString("b1");
        perPassBuffer->field = firstPerPassDeclaration;

        // Add buffer to statements.
        AddSingleStatement(root, statementBeforeBuffers, perPassBuffer);
    }
}

TreeDump::TreeDump()
    : indent(0)
{
}

void TreeDump::VisitDeclaration(HLSLDeclaration* node)
{
    HLSLDeclaration* decl = static_cast<HLSLDeclaration*>(node);
    const char* dtype = "uniform";

    if (decl->type.baseType == HLSLBaseType_Sampler || decl->type.baseType == HLSLBaseType_Sampler2D || decl->type.baseType == HLSLBaseType_SamplerCube)
        dtype = "sampler";

    if (decl->type.flags & HLSLTypeFlag_Property)
        dtype = "property";

    Logger::Info("%s[decl.%s]  \"%s\"  : %s", _IndentString(indent), dtype, node->name, _TypeName(decl->type));

    if (decl->assignment)
    {
        if (decl->assignment->nodeType == HLSLNodeType_ArrayAccess)
        {
            HLSLArrayAccess* aa = static_cast<HLSLArrayAccess*>(decl->assignment);

            if (aa->array->nodeType == HLSLNodeType_IdentifierExpression)
            {
                HLSLIdentifierExpression* ie = static_cast<HLSLIdentifierExpression*>(aa->array);

                Logger::Info("    [array-access]  \"%s\"", ie->name);
            }

            //-            M4::Log_Error("    aa  \"%s\"\n", aa->fileName);
        }
        else if (decl->assignment->nodeType == HLSLNodeType_MemberAccess)
        {
            HLSLMemberAccess* ma = static_cast<HLSLMemberAccess*>(decl->assignment);

            Logger::Info("    [member-access]  \"%s\"", ma->field);
        }
        else if (decl->assignment->nodeType == HLSLNodeType_ConstructorExpression)
        {
            HLSLConstructorExpression* ctor = static_cast<HLSLConstructorExpression*>(decl->assignment);
            const char* tname = "";

            Logger::Info("    [ctor]  \"%s\"", tname);
        }
    }
    HLSLTreeVisitor::VisitDeclaration(node);
}

void TreeDump::VisitStruct(HLSLStruct* node)
{
    Logger::Info("%s[struct]  \"%s\"", _IndentString(indent), node->name);
    ++indent;
    HLSLTreeVisitor::VisitStruct(node);
    --indent;
}

void TreeDump::VisitStructField(HLSLStructField* node)
{
    HLSLStructField* field = static_cast<HLSLStructField*>(node);

    Logger::Info("%s[field]  \"%s\" : %s  (%s)", _IndentString(indent), field->name, _TypeName(field->type), (field->semantic) ? field->semantic : "<no-semantics>");

    HLSLTreeVisitor::VisitStructField(node);
}

void TreeDump::VisitFunction(HLSLFunction* func)
{
    Logger::Info("%s[function]  \"%s\"", _IndentString(indent), func->name);

    for (HLSLStatement* s = func->statement; s; s = s->nextStatement)
    {
        _DumpStatement(s, indent + 1);
    }
}

const char* TreeDump::_IndentString(int indent)
{
    static char text[256];

    memset(text, ' ', indent * 2);
    text[indent * 2] = '\0';
    return text;
}

void TreeDump::_DumpStatement(HLSLStatement* s, int indent)
{
    switch (s->nodeType)
    {
    case HLSLNodeType_Declaration:
    {
        HLSLDeclaration* decl = static_cast<HLSLDeclaration*>(s);
        const char* dtype = "uniform";

        if (decl->type.baseType == HLSLBaseType_Sampler || decl->type.baseType == HLSLBaseType_Sampler2D || decl->type.baseType == HLSLBaseType_SamplerCube)
            dtype = "sampler";

        if (decl->type.flags & HLSLTypeFlag_Property)
            dtype = "property";

        Logger::Info("%s[decl.%s]  \"%s\"  : %s", _IndentString(indent), dtype, decl->name, _TypeName(decl->type));

        for (HLSLExpression* e = decl->assignment; e; e = e->nextExpression)
        {
            _DumpExpression(e, indent + 1);
        }
    }
    break;

    case HLSLNodeType_Expression:
    {
        Logger::Info("  expr");
    }
    break;

    case HLSLNodeType_ExpressionStatement:
    {
        HLSLExpressionStatement* st = static_cast<HLSLExpressionStatement*>(s);

        Logger::Info("%s[expr.statement]", _IndentString(indent));
        _DumpExpression(st->expression, indent + 1);
    }
    break;

    case HLSLNodeType_ReturnStatement:
    {
        HLSLReturnStatement* ret = static_cast<HLSLReturnStatement*>(s);

        Logger::Info("%s[return]", _IndentString(indent));

        for (HLSLExpression* e = ret->expression; e; e = e->nextExpression)
        {
            _DumpExpression(e, indent + 1);
        }
    }
    break;

    case HLSLNodeType_BlockStatement:
    {
        HLSLBlockStatement* block = static_cast<HLSLBlockStatement*>(s);

        Logger::Info("%s[block]", _IndentString(indent));
        for (HLSLStatement* b = block->statement; b; b = b->nextStatement)
        {
            _DumpStatement(b, indent + 1);
        }
        /*
            Log_Error( "%sblock\n", _IndentString(indent) );
            for( HLSLStatement* b=block->statement; b; b=b->nextStatement )
            {
                for( HLSLAttribute* a=block->attributes; a; a=a->nextAttribute )
                {
                    if( a->argument )
                        _DumpExpression( a->argument, indent+1, false );
                }
            }
*/
    }
    break;
    default:
        break; // to shut up goddamn warning
    }
}

void TreeDump::_DumpExpression(HLSLExpression* expr, int indent, bool dump_subexpr)
{
    switch (expr->nodeType)
    {
    case HLSLNodeType_ConstructorExpression:
    {
        HLSLConstructorExpression* ctor = static_cast<HLSLConstructorExpression*>(expr);

        Logger::Info("%s[ctor]", _IndentString(indent));
        _DumpExpression(ctor->argument, indent + 1);
    }
    break;

    case HLSLNodeType_LiteralExpression:
    {
        HLSLLiteralExpression* li = static_cast<HLSLLiteralExpression*>(expr);

        switch (li->type)
        {
        case HLSLBaseType_Float:
        case HLSLBaseType_Half:
            Logger::Info("%s[literal] %f", _IndentString(indent), li->fValue);
            break;
        case HLSLBaseType_Int:
            Logger::Info("%s[literal] %i", _IndentString(indent), li->iValue);
            break;
        case HLSLBaseType_Uint:
            Logger::Info("%s[literal] %u", _IndentString(indent), unsigned(li->iValue));
            break;
        default:
            break; // to shut up goddamn warning
        }
    }
    break;

    case HLSLNodeType_BinaryExpression:
    {
        HLSLBinaryExpression* bin = static_cast<HLSLBinaryExpression*>(expr);
        const char* op = "";

        switch (bin->binaryOp)
        {
        case HLSLBinaryOp_And:
            op = "and";
            break;
        case HLSLBinaryOp_Or:
            op = "or";
            break;
        case HLSLBinaryOp_Add:
            op = "add";
            break;
        case HLSLBinaryOp_Sub:
            op = "sub";
            break;
        case HLSLBinaryOp_Mul:
            op = "mul";
            break;
        case HLSLBinaryOp_Div:
            op = "div";
            break;
        case HLSLBinaryOp_Less:
            op = "less";
            break;
        case HLSLBinaryOp_Greater:
            op = "greater";
            break;
        case HLSLBinaryOp_LessEqual:
            op = "lesseq";
            break;
        case HLSLBinaryOp_GreaterEqual:
            op = "gequal";
            break;
        case HLSLBinaryOp_Equal:
            op = "equal";
            break;
        case HLSLBinaryOp_NotEqual:
            op = "nequal";
            break;
        case HLSLBinaryOp_BitAnd:
            op = "bit-and";
            break;
        case HLSLBinaryOp_BitOr:
            op = "bit-or";
            break;
        case HLSLBinaryOp_BitXor:
            op = "bit-xor";
            break;
        case HLSLBinaryOp_Assign:
            op = "assign";
            break;
        case HLSLBinaryOp_AddAssign:
            op = "add-assign";
            break;
        case HLSLBinaryOp_SubAssign:
            op = "sub-assign";
            break;
        case HLSLBinaryOp_MulAssign:
            op = "mul-assign";
            break;
        case HLSLBinaryOp_DivAssign:
            op = "div-assign";
            break;
        }

        Logger::Info("%s[bin.expr] %s", _IndentString(indent), op);
        Logger::Info("%sarg1", _IndentString(indent + 1));
        _DumpExpression(bin->expression1, indent + 2);
        Logger::Info("%sarg2", _IndentString(indent + 1));
        _DumpExpression(bin->expression2, indent + 2);
    }
    break;

    case HLSLNodeType_IdentifierExpression:
    {
        HLSLIdentifierExpression* var = static_cast<HLSLIdentifierExpression*>(expr);

        Logger::Info("%s[identifier] \"%s\"", _IndentString(indent), var->name);
    }
    break;

    case HLSLNodeType_MemberAccess:
    {
        HLSLMemberAccess* member = static_cast<HLSLMemberAccess*>(expr);

        Logger::Info("%s[member.access] \"%s\"", _IndentString(indent), member->field);
        _DumpExpression(member->object, indent + 1, false);
    }
    break;

    case HLSLNodeType_FunctionCall:
    {
        HLSLFunctionCall* call = static_cast<HLSLFunctionCall*>(expr);

        Logger::Info("%s[call] \"%s\"", _IndentString(indent), call->function->name);
        int a = 0;
        for (HLSLExpression *e = call->argument; e; e = e->nextExpression, ++a)
        {
            Logger::Info("%sarg%i", _IndentString(indent + 1), 1 + a);
            _DumpExpression(e, indent + 2, false);
        }
    }
    break;
    default:
        break; // to shut up goddamn warning
    }

    if (dump_subexpr)
    {
        for (HLSLExpression* e = expr->nextExpression; e; e = e->nextExpression)
        {
            _DumpExpression(e, indent + 1);
        }
    }
}

const char*
TreeDump::_TypeName(const HLSLType& type)
{
    switch (type.baseType)
    {
    case HLSLBaseType_Void:
        return "void";
    case HLSLBaseType_Float:
        return "float";
    case HLSLBaseType_Float2:
        return "float2";
    case HLSLBaseType_Float3:
        return "float3";
    case HLSLBaseType_Float4:
        return "float4";
    case HLSLBaseType_Float3x3:
        return "float3x3";
    case HLSLBaseType_Float4x4:
        return "float4x4";
    case HLSLBaseType_Half:
        return "half";
    case HLSLBaseType_Half2:
        return "half2";
    case HLSLBaseType_Half3:
        return "half3";
    case HLSLBaseType_Half4:
        return "half4";
    case HLSLBaseType_Half3x3:
        return "half3x3";
    case HLSLBaseType_Half4x4:
        return "half4x4";
    case HLSLBaseType_Bool:
        return "bool";
    case HLSLBaseType_Int:
        return "int";
    case HLSLBaseType_Int2:
        return "int2";
    case HLSLBaseType_Int3:
        return "int3";
    case HLSLBaseType_Int4:
        return "int4";
    case HLSLBaseType_Uint:
        return "uint";
    case HLSLBaseType_Uint2:
        return "uint2";
    case HLSLBaseType_Uint3:
        return "uint3";
    case HLSLBaseType_Uint4:
        return "uint4";
    case HLSLBaseType_Texture:
        return "texture";
    case HLSLBaseType_Sampler:
        return "sampler";
    case HLSLBaseType_Sampler2D:
        return "sampler2D";
    case HLSLBaseType_Sampler3D:
        return "sampler3D";
    case HLSLBaseType_SamplerCube:
        return "samplerCUBE";
    case HLSLBaseType_UserDefined:
        return type.typeName;
    default:
        break; // to shut up goddamn warning
    }
    return "?";
}

} // namespace sl
