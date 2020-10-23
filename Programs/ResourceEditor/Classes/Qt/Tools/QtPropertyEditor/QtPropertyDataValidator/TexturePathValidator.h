#ifndef __RESOURCEEDITORQT__TEXTUREPATHVALIDATOR__
#define __RESOURCEEDITORQT__TEXTUREPATHVALIDATOR__

#include "PathValidator.h"

class TexturePathValidator : public PathValidator
{
public:
    TexturePathValidator(const QStringList& value);

protected:
    virtual bool ValidateInternal(const QVariant& v);
    virtual void FixupInternal(QVariant& v) const;
};

#endif /* defined(__RESOURCEEDITORQT__TEXTUREPATHVALIDATOR__) */
