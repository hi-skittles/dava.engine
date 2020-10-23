#include "LayoutFormula.h"

namespace DAVA
{
LayoutFormula::LayoutFormula()
{
    formula.Parse(source);
}

LayoutFormula::~LayoutFormula() = default;

const String& LayoutFormula::GetSource() const
{
    return source;
}

void LayoutFormula::SetSource(const String& str)
{
    source = str;
    errorMsg = "";
    if (!formula.Parse(source))
    {
        errorMsg = formula.GetParsingError();
    }
    hasChanges = true;
}

bool LayoutFormula::HasError() const
{
    return !errorMsg.empty();
}

bool LayoutFormula::IsEmpty() const
{
    return errorMsg.empty() && !formula.IsValid();
}

bool LayoutFormula::HasChanges() const
{
    return hasChanges;
}

void LayoutFormula::ResetChanges()
{
    hasChanges = false;
}

void LayoutFormula::MarkChanges()
{
    hasChanges = true;
}

float32 LayoutFormula::Calculate(const Reflection& ref)
{
    if (formula.IsValid() && errorMsg.empty())
    {
        Any res = formula.Calculate(ref);

        if (res.CanCast<float32>())
        {
            return res.Cast<float32>();
        }
        else if (res.CanCast<int32>())
        {
            return static_cast<float32>(res.Cast<int32>());
        }
        else if (res.IsEmpty())
        {
            errorMsg = formula.GetCalculationError();
            hasChanges = true;

            DVASSERT(!errorMsg.empty());
        }
    }
    return 0.0f;
}

const String& LayoutFormula::GetErrorMessage() const
{
    return errorMsg;
}
}
