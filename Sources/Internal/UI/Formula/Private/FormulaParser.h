#pragma once

#include "Base/BaseTypes.h"

#include "UI/Formula/Private/FormulaExpression.h"
#include "UI/Formula/Private/FormulaData.h"
#include "UI/Formula/Private/FormulaException.h"
#include "UI/Formula/Private/FormulaTokenizer.h"

namespace DAVA
{
/**
 \ingroup formula
 
 FormualParser stores string with expressions and allow to read sequences
 of expressions.
 */
class FormulaParser final
{
public:
    FormulaParser(const String& str);
    ~FormulaParser();

    /**
     \ingroup formula
     
     Method read one expression.
     */
    std::shared_ptr<FormulaExpression> ParseExpression();

    /**
     \ingroup formula
     
     Method read map of expressions or other data. 
     Format:
     key1 = expression
     key2 = data
     key3 = otherExpression
     */
    std::shared_ptr<FormulaDataMap> ParseMap();

private:
    std::shared_ptr<FormulaDataVector> ParseVector();
    std::shared_ptr<FormulaExpression> ParseConditionalExpression();
    std::shared_ptr<FormulaExpression> ParseLogicalOr();
    std::shared_ptr<FormulaExpression> ParseLogicalAnd();
    std::shared_ptr<FormulaExpression> ParseEquality();
    std::shared_ptr<FormulaExpression> ParseRelation();
    std::shared_ptr<FormulaExpression> ParseAdditive();
    std::shared_ptr<FormulaExpression> ParseMultiplicative();
    std::shared_ptr<FormulaExpression> ParseUnary();
    std::shared_ptr<FormulaExpression> ParsePostfix();
    std::shared_ptr<FormulaExpression> ParsePrimary();
    std::shared_ptr<FormulaExpression> ParseFunction(const String& identifier, int32 lineNumber, int32 positionInLine);
    std::shared_ptr<FormulaExpression> ParseAccess(const String& identifier, int32 lineNumber, int32 positionInLine);

    FormulaToken LookToken();
    FormulaToken NextToken();
    bool IsIdentifier(const FormulaToken& token, const String& identifier) const;
    String GetTokenStringValue(const FormulaToken& token) const;
    FormulaBinaryOperatorExpression::Operator TokenTypeToBinaryOp(FormulaToken::Type type);

    FormulaToken token;
    FormulaTokenizer tokenizer;
};
}
