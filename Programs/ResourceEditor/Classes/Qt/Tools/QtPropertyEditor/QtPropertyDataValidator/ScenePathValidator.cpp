#include "ScenePathValidator.h"

ScenePathValidator::ScenePathValidator(const QStringList& value)
    : PathValidator(value)
{
}

bool ScenePathValidator::ValidateInternal(const QVariant& v)
{
    bool res = RegExpValidator::ValidateInternal(v);

    QString val = v.toString();
    if (res && val != "")
    {
        res = val.endsWith(".sc2");
    }

    return res;
}
