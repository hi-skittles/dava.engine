#pragma once

#include "Base/BaseTypes.h"
#include "Base/Any.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class FormulaExpression;
class FormulaContext;

/**
 \ingroup formula
 
 Formula represents facade to simple access to parsing, evaluating and
 formatting formulas. Developed with love.
 */
class Formula final
{
public:
    Formula();
    ~Formula();

    /**
     Parses string in c like format to internal representation.
     */
    bool Parse(const String& str);

    /**
     Reset internal state.
     */
    void Reset();

    /**
     Indicates that formula is parsed and ready to evaluation.
     */
    bool IsValid() const;

    /**
     Calculates prepared formula with data from context.
     */
    Any Calculate(const std::shared_ptr<FormulaContext>& context);

    /**
     Calculates prepared formula with data from reflection.
     */
    Any Calculate(const Reflection& ref);

    /**
     Non empty result indicates that errors was occured on parsing step.
     */
    const String& GetParsingError() const;

    /**
     Non empty result indicates that errors was occured on calculation step.
     */
    const String& GetCalculationError() const;

    /**
     Formats formula to string representation.
     */
    String ToString() const;

private:
    std::shared_ptr<FormulaExpression> exp;

    String parsingError;
    String calculationError;
};
}
