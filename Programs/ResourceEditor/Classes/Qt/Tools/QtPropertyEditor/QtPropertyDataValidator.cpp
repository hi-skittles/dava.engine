#include "QtPropertyDataValidator.h"

bool QtPropertyDataValidator::Validate(QVariant& v)
{
    bool ret = ValidateInternal(v);
    if (!ret)
    {
        FixupInternal(v);
        ret = ValidateInternal(v);
        if (!ret)
        {
            ErrorNotifyInternal(v);
        }
    }
    return ret;
}