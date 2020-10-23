#ifndef __QT_PROPERTY_DATA_VALIDATOR_H__
#define __QT_PROPERTY_DATA_VALIDATOR_H__

#include <QVariant>

class QtPropertyDataValidator
{
public:
    bool Validate(QVariant& v);

    virtual ~QtPropertyDataValidator(){};

protected:
    virtual bool ValidateInternal(const QVariant& v) = 0;
    virtual void FixupInternal(QVariant& v) const {};

    virtual void ErrorNotifyInternal(const QVariant& v) const
    {
    }
};

#endif // __QT_PROPERTY_DATA_VALIDATOR_H__
