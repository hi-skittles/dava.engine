#ifndef __INT_RANGE_VALIDATOR_H__
#define __INT_RANGE_VALIDATOR_H__

#include "../QtPropertyDataValidator.h"
#include <QIntValidator>

class IntRangeValidator : public QtPropertyDataValidator
{
public:
    IntRangeValidator(int minValue, int maxValue);

    void SetRange(int minValue, int maxValue);

    int GetMaximum() const;
    void SetMaximum(int maxValue);

    int GetMinimum() const;
    void SetMinimum(int minValue);

protected:
    virtual bool ValidateInternal(const QVariant& v);

private:
    QIntValidator innerValidator;
};

#endif // __INT_RANGE_VALIDATOR_H__
