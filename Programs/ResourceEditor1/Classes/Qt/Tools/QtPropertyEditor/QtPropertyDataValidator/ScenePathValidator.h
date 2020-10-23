#ifndef __RESOURCEEDITORQT__SCENEPATHVALIDATOR__
#define __RESOURCEEDITORQT__SCENEPATHVALIDATOR__

#include "PathValidator.h"

class ScenePathValidator : public PathValidator
{
public:
    ScenePathValidator(const QStringList& value);

protected:
    virtual bool ValidateInternal(const QVariant& v);
};

#endif /* defined(__RESOURCEEDITORQT__SCENEPATHVALIDATOR__) */
