#ifndef __FLOAT_RANGE_VALIDATOR_H__
#define __FLOAT_RANGE_VALIDATOR_H__

#include "../QtPropertyDataValidator.h"
#include <QDoubleValidator>

class FloatRangeValidator : public QtPropertyDataValidator
{
public:
    FloatRangeValidator(float minValue, float maxValue);

    void SetRange(float minValue, float maxValue);

    int GetMaximum() const;
    void SetMaximum(float maxValue);

    int GetMinimum() const;
    void SetMinimum(float minValue);

protected:
    virtual bool ValidateInternal(const QVariant& v);

private:
    QDoubleValidator innerValidator;
};

#endif // __FLOAT_RANGE_VALIDATOR_H__
