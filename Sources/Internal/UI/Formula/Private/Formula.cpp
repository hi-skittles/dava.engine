#include "UI/Formula/Formula.h"

#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaExecutor.h"
#include "UI/Formula/Private/FormulaFormatter.h"

namespace DAVA
{
Formula::Formula()
{
}

Formula::~Formula()
{
}

bool Formula::Parse(const String& str)
{
    Reset();

    try
    {
        FormulaParser parser(str);
        exp = parser.ParseExpression();
        return true;
    }
    catch (const FormulaException& error)
    {
        parsingError = error.GetFormattedMessage();
        return false;
    }
}

void Formula::Reset()
{
    exp.reset();
    parsingError = "";
    calculationError = "";
}

bool Formula::IsValid() const
{
    return exp.get() != nullptr;
}

Any Formula::Calculate(const std::shared_ptr<FormulaContext>& context)
{
    calculationError = "";

    if (exp)
    {
        try
        {
            FormulaExecutor executor(context);
            return executor.Calculate(exp.get());
        }
        catch (const FormulaException& error)
        {
            calculationError = error.GetFormattedMessage();
        }
    }

    return Any();
}

Any Formula::Calculate(const Reflection& ref)
{
    calculationError = "";

    if (exp)
    {
        try
        {
            std::shared_ptr<FormulaReflectionContext> context = std::make_shared<FormulaReflectionContext>(ref, nullptr);
            return Calculate(context);
        }
        catch (const FormulaException& error)
        {
            calculationError = error.GetFormattedMessage();
        }
    }

    return Any();
}

const String& Formula::GetParsingError() const
{
    return parsingError;
}

const String& Formula::GetCalculationError() const
{
    return calculationError;
}

String Formula::ToString() const
{
    if (exp)
    {
        return FormulaFormatter().Format(exp.get());
    }
    return "";
}
}
