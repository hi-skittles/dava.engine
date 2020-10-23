#ifndef __RESOURCEEDITORQT__TILEMASKEDITORCOMMANDS__
#define __RESOURCEEDITORQT__TILEMASKEDITORCOMMANDS__

#include "Commands2/Base/RECommand.h"

#include "Base/FastName.h"
#include "Math/Color.h"
#include "Math/Rect.h"

class LandscapeProxy;
class SceneEditor2;

namespace DAVA
{
class Entity;
class Image;
class Texture;
}

class ModifyTilemaskCommand : public RECommand
{
public:
    ModifyTilemaskCommand(LandscapeProxy* landscapeProxy, const DAVA::Rect& updatedRect);
    ~ModifyTilemaskCommand() override;

    void Undo() override;
    void Redo() override;

protected:
    DAVA::Image* undoImageMask = nullptr;
    DAVA::Image* redoImageMask = nullptr;
    LandscapeProxy* landscapeProxy = nullptr;
    DAVA::Rect updatedRect;

    void ApplyImageToTexture(DAVA::Image* image, DAVA::Texture* dstTex);
};

class SetTileColorCommand : public RECommand
{
public:
    SetTileColorCommand(LandscapeProxy* landscapeProxy, const DAVA::FastName& level, const DAVA::Color& color);
    ~SetTileColorCommand() override;

    void Undo() override;
    void Redo() override;

protected:
    const DAVA::FastName& level;
    DAVA::Color redoColor;
    DAVA::Color undoColor;
    LandscapeProxy* landscapeProxy = nullptr;
};

#endif /* defined(__RESOURCEEDITORQT__TILEMASKEDITORCOMMANDS__) */
