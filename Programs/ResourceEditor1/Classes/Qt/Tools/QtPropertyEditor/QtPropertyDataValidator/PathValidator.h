#ifndef __PATH_VALIDATOR_H__
#define __PATH_VALIDATOR_H__

#include "RegExpValidator.h"
#include "Utils/StringFormat.h"
#include <QStringList>

class PathValidator : public RegExpValidator
{
public:
    PathValidator(const QStringList& value);

protected:
    QStringList referencePathList;

    virtual void ErrorNotifyInternal(const QVariant& v) const;

    virtual DAVA::String PrepareErrorMessage(const QVariant& v) const;
};

#endif // __PATH_VALIDATOR_H__
