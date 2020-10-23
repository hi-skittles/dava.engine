#ifndef __QT_REG_EXP_VALIDATOR_H__
#define __QT_REG_EXP_VALIDATOR_H__

#include "../QtPropertyDataValidator.h"
#include <QRegExpValidator>

class RegExpValidator : public QtPropertyDataValidator
{
public:
    RegExpValidator(const QString& value);

    QString GetRegularExpression() const;

    void SetRegularExpression(const QString& value);

protected:
    virtual bool ValidateInternal(const QVariant& v);

private:
    QRegExpValidator innerValidator;
};

#endif // __QT_REG_EXP_VALIDATOR_H__
