#ifndef __RESOURCEEDITORQT__RULERTOOLPROXY__
#define __RESOURCEEDITORQT__RULERTOOLPROXY__

#include "DAVAEngine.h"

class RulerToolProxy : public DAVA::BaseObject
{
protected:
    ~RulerToolProxy();

public:
    RulerToolProxy(DAVA::int32 size);

    DAVA::int32 GetSize();

    DAVA::Texture* GetTexture();

protected:
    DAVA::Texture* rulerToolTexture;
    DAVA::int32 size;
    bool spriteChanged;
};

#endif /* defined(__RESOURCEEDITORQT__RULERTOOLPROXY__) */
