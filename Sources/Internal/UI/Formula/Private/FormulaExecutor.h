#pragma once

#include "Base/BaseTypes.h"
#include "UI/Formula/Private/FormulaExpression.h"
#include "UI/Formula/FormulaContext.h"

namespace DAVA
{
/**
 \ingroup formula

 Executor can calculate expression. It uses context to get data access.
 */
class FormulaExecutor : private FormulaExpressionVisitor
{
public:
    FormulaExecutor(const std::shared_ptr<FormulaContext>& context);
    ~FormulaExecutor() override;

    /**
     \ingroup formula
     
     Method calculates expression and returns result.
     */
    Any Calculate(FormulaExpression* exp);

    /**
     \ingroup formula

     Method calculates expression and returns reference to data instead of value.
     It is useful if need found reference to data from context to store data.
     */
    Reflection GetDataReference(FormulaExpression* exp);

    /**
     \ingroup formula
     
     Vector of pointers is just way to know from which variables this expression
     is dependent.
     
     It can be useful for data binding system purposes. Data binding system
     uses pointers as marks to have posibility to know that some data was
     changed and it should recalculate dependent formulas.
     
     */
    const Vector<void*>& GetDependencies() const;

private:
    void Visit(FormulaValueExpression* exp) override;
    void Visit(FormulaNegExpression* exp) override;
    void Visit(FormulaNotExpression* exp) override;
    void Visit(FormulaWhenExpression* exp) override;
    void Visit(FormulaBinaryOperatorExpression* exp) override;
    void Visit(FormulaFunctionExpression* exp) override;
    void Visit(FormulaFieldAccessExpression* exp) override;
    void Visit(FormulaIndexExpression* exp) override;

    const Any& CalculateImpl(FormulaExpression* exp);
    const Reflection& GetDataReferenceImpl(FormulaExpression* exp);

    template <typename T>
    Any CalculateNumberAnyValues(FormulaBinaryOperatorExpression::Operator op, Any lVal, Any rVal) const;

    template <typename T>
    Any CalculateIntAnyValues(FormulaBinaryOperatorExpression::Operator op, Any lVal, Any rVal, FormulaExpression* exp) const;

    template <typename T>
    Any CalculateIntValues(FormulaBinaryOperatorExpression::Operator op, T lVal, T rVal, FormulaExpression* exp) const;

    template <typename T>
    Any CalculateNumberValues(FormulaBinaryOperatorExpression::Operator op, T lVal, T rVal) const;

    bool CastToInt32(const Any& val, int32* res) const;

    std::shared_ptr<FormulaContext> context;
    Any calculationResult;
    Reflection dataReference;
    Vector<void*> dependencies;
    Vector<FormulaExpression*> expressionsStack;
};
}
