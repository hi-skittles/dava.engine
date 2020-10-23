#include "IntRangeValidator.h"

IntRangeValidator::IntRangeValidator(int minValue, int maxValue)
{
    innerValidator.setRange(minValue, maxValue);
}

bool IntRangeValidator::ValidateInternal(const QVariant& value)
{
    QString validateValue = value.toString();
    int pos = 0;
    return innerValidator.validate(validateValue, pos) == QValidator::Acceptable;
}

void IntRangeValidator::SetRange(int minValue, int maxValue)
{
    innerValidator.setRange(minValue, maxValue);
}

int IntRangeValidator::GetMaximum() const
{
    return innerValidator.top();
}

void IntRangeValidator::SetMaximum(int maxValue)
{
    innerValidator.setTop(maxValue);
}

int IntRangeValidator::GetMinimum() const
{
    return innerValidator.bottom();
}

void IntRangeValidator::SetMinimum(int minValue)
{
    innerValidator.setBottom(minValue);
}
