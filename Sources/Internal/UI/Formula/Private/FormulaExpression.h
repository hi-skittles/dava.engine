#pragma once

#include "Base/BaseTypes.h"
#include "Base/Any.h"

namespace DAVA
{
class FormulaValueExpression;
class FormulaNegExpression;
class FormulaNotExpression;
class FormulaWhenExpression;
class FormulaBinaryOperatorExpression;
class FormulaFunctionExpression;
class FormulaFieldAccessExpression;
class FormulaIndexExpression;

class FormulaExpressionVisitor
{
public:
    virtual ~FormulaExpressionVisitor()
    {
    }

    virtual void Visit(FormulaValueExpression* exp) = 0;
    virtual void Visit(FormulaNegExpression* exp) = 0;
    virtual void Visit(FormulaNotExpression* exp) = 0;
    virtual void Visit(FormulaWhenExpression* exp) = 0;
    virtual void Visit(FormulaBinaryOperatorExpression* exp) = 0;
    virtual void Visit(FormulaFunctionExpression* exp) = 0;
    virtual void Visit(FormulaFieldAccessExpression* exp) = 0;
    virtual void Visit(FormulaIndexExpression* exp) = 0;
};

/**
 \ingroup formula

 FormulaExpression represents node in AST.  
 */
class FormulaExpression
{
public:
    FormulaExpression(int32 lineNumber, int32 positionInLine);
    virtual ~FormulaExpression();

    virtual bool IsValue() const;

    virtual void Accept(FormulaExpressionVisitor* visitor) = 0;

    int32 GetLineNumber() const;
    int32 GetPositionInLine() const;

private:
    int32 lineNumber = 0;
    int32 positionInLine = 0;
};

/**
 \ingroup formula
 
 FormulaValueExpression stores data.
 */
class FormulaValueExpression : public FormulaExpression
{
public:
    FormulaValueExpression(const Any& value, int32 lineNumber, int32 positionInLine);

    bool IsValue() const override;
    const Any& GetValue() const;

    void Accept(FormulaExpressionVisitor* visitor) override;

private:
    Any value;
};

/**
 \ingroup formula
 
 Stores node for unary minus operator.
 */
class FormulaNegExpression : public FormulaExpression
{
public:
    FormulaNegExpression(const std::shared_ptr<FormulaExpression>& exp, int32 lineNumber, int32 positionInLine);

    void Accept(FormulaExpressionVisitor* visitor) override;
    FormulaExpression* GetExp() const;

private:
    std::shared_ptr<FormulaExpression> exp;
};

/**
 \ingroup formula
 
 Stores node for unary logical not operator.
 */
class FormulaNotExpression : public FormulaExpression
{
public:
    FormulaNotExpression(const std::shared_ptr<FormulaExpression>& exp, int32 lineNumber, int32 positionInLine);

    void Accept(FormulaExpressionVisitor* visitor) override;

    FormulaExpression* GetExp() const;

private:
    std::shared_ptr<FormulaExpression> exp;
};

/**
 \ingroup formula
 
 Stores node for conditional when operator (select for execution only one branch).
 */
class FormulaWhenExpression : public FormulaExpression
{
public:
    FormulaWhenExpression(const Vector<std::pair<std::shared_ptr<FormulaExpression>, std::shared_ptr<FormulaExpression>>>& branches,
                          std::shared_ptr<FormulaExpression> elseBranch,
                          int32 lineNumber, int32 positionInLine);

    void Accept(FormulaExpressionVisitor* visitor) override;

    const Vector<std::pair<std::shared_ptr<FormulaExpression>, std::shared_ptr<FormulaExpression>>>& GetBranches() const;
    FormulaExpression* GetElseBranch() const;

private:
    Vector<std::pair<std::shared_ptr<FormulaExpression>, std::shared_ptr<FormulaExpression>>> branches;
    std::shared_ptr<FormulaExpression> elseBranch;
};

/**
 \ingroup formula
 
 Stores node for binary operator.
 */
class FormulaBinaryOperatorExpression : public FormulaExpression
{
public:
    enum Operator
    {
        OP_PLUS,
        OP_MINUS,
        OP_MUL,
        OP_DIV,
        OP_MOD,
        OP_AND,
        OP_OR,
        OP_EQ,
        OP_NOT_EQ,
        OP_LE,
        OP_LT,
        OP_GE,
        OP_GT
    };

    FormulaBinaryOperatorExpression(Operator op, const std::shared_ptr<FormulaExpression>& lhs, const std::shared_ptr<FormulaExpression>& rhs, int32 lineNumber, int32 positionInLine);

    void Accept(FormulaExpressionVisitor* visitor) override;

    Operator GetOperator() const;
    int32 GetOperatorPriority() const;

    FormulaExpression* GetLhs() const;
    FormulaExpression* GetRhs() const;

private:
    Operator op;
    std::shared_ptr<FormulaExpression> lhs;
    std::shared_ptr<FormulaExpression> rhs;
};

/**
 \ingroup formula
 
 Stores node for function with parameters.
 */
class FormulaFunctionExpression : public FormulaExpression
{
public:
    FormulaFunctionExpression(const String& name, const Vector<std::shared_ptr<FormulaExpression>>& params, int32 lineNumber, int32 positionInLine);

    void Accept(FormulaExpressionVisitor* visitor) override;

    const String& GetName() const;
    const Vector<std::shared_ptr<FormulaExpression>>& GetParms() const;

private:
    String name;
    Vector<std::shared_ptr<FormulaExpression>> params;
};

/**
 \ingroup formula
 
 Stores node for access to fields. Null expression indicates that data will be 
 taken from the context.
 */
class FormulaFieldAccessExpression : public FormulaExpression
{
public:
    FormulaFieldAccessExpression(const std::shared_ptr<FormulaExpression>& exp, const String& fieldName, int32 lineNumber, int32 positionInLine);

    void Accept(FormulaExpressionVisitor* visitor) override;

    FormulaExpression* GetExp() const;
    const String& GetFieldName() const;

private:
    std::shared_ptr<FormulaExpression> exp;
    String fieldName;
};

/**
 \ingroup formula
 
 Stores node for index access to fields.
 */
class FormulaIndexExpression : public FormulaExpression
{
public:
    FormulaIndexExpression(const std::shared_ptr<FormulaExpression>& exp, const std::shared_ptr<FormulaExpression>& indexExp, int32 lineNumber, int32 positionInLine);

    void Accept(FormulaExpressionVisitor* visitor) override;

    FormulaExpression* GetExp() const;
    FormulaExpression* GetIndexExp() const;

private:
    std::shared_ptr<FormulaExpression> exp;
    std::shared_ptr<FormulaExpression> indexExp;
};
}
