#include "RegExpValidator.h"

RegExpValidator::RegExpValidator(const QString& value)
{
    SetRegularExpression(value);
}

bool RegExpValidator::ValidateInternal(const QVariant& value)
{
    QString validateValue = value.toString();
    int pos = 0;
    return innerValidator.validate(validateValue, pos) == QValidator::Acceptable;
}

QString RegExpValidator::GetRegularExpression() const
{
    return innerValidator.regExp().pattern();
}

void RegExpValidator::SetRegularExpression(const QString& value)
{
    innerValidator.setRegExp(QRegExp(value));
}
