#pragma once

#include "UI/Formula/Private/FormulaExpression.h"

namespace DAVA
{
/**
 \ingroup formula
 
 Helper for converting expression back to text form.
 */
class FormulaFormatter : private FormulaExpressionVisitor
{
public:
    FormulaFormatter();
    ~FormulaFormatter() override;

    String Format(FormulaExpression* exp);

    static String AnyToString(const Any& val);
    static String AnyTypeToString(const Any& val);
    static String BinaryOpToString(FormulaBinaryOperatorExpression::Operator op);

private:
    void Visit(FormulaValueExpression* exp) override;
    void Visit(FormulaNegExpression* exp) override;
    void Visit(FormulaNotExpression* exp) override;
    void Visit(FormulaWhenExpression* exp) override;
    void Visit(FormulaBinaryOperatorExpression* exp) override;
    void Visit(FormulaFunctionExpression* exp) override;
    void Visit(FormulaFieldAccessExpression* exp) override;
    void Visit(FormulaIndexExpression* exp) override;

    int32 GetExpPriority(FormulaExpression* exp) const;

    StringStream stream;
};
}
