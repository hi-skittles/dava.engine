#include "FloatRangeValidator.h"

FloatRangeValidator::FloatRangeValidator(float minValue, float maxValue)
{
    innerValidator.setRange(minValue, maxValue);
}

bool FloatRangeValidator::ValidateInternal(const QVariant& value)
{
    QString validateValue = value.toString();
    int pos = 0;
    return innerValidator.validate(validateValue, pos) == QValidator::Acceptable;
}

void FloatRangeValidator::SetRange(float minValue, float maxValue)
{
    innerValidator.setRange(minValue, maxValue);
}

int FloatRangeValidator::GetMaximum() const
{
    return innerValidator.top();
}

void FloatRangeValidator::SetMaximum(float maxValue)
{
    innerValidator.setTop(maxValue);
}

int FloatRangeValidator::GetMinimum() const
{
    return innerValidator.bottom();
}

void FloatRangeValidator::SetMinimum(float minValue)
{
    innerValidator.setBottom(minValue);
}
