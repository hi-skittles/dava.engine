#ifndef __TEXTURE_FILE_VALIDATOR_H__
#define __TEXTURE_FILE_VALIDATOR_H__

#include "PathValidator.h"
#include <QStringList>
#include "DAVAEngine.h"

class HeightMapValidator : public PathValidator
{
public:
    HeightMapValidator(const QStringList& value);

protected:
    virtual bool ValidateInternal(const QVariant& v);

    virtual void ErrorNotifyInternal(const QVariant& v) const;

    DAVA::String notifyMessage;
};

#endif // __TEXTURE_FILE_VALIDATOR_H__
