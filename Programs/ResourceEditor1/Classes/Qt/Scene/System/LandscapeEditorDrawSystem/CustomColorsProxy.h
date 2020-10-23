#ifndef __RESOURCEEDITORQT__CUSTOMCOLORSPROXY__
#define __RESOURCEEDITORQT__CUSTOMCOLORSPROXY__

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Math/Rect.h"

namespace DAVA
{
class Texture;
class NMaterial;
}

class CustomColorsProxy : public DAVA::BaseObject
{
protected:
    ~CustomColorsProxy();

public:
    CustomColorsProxy(DAVA::int32 size);

    DAVA::Texture* GetTexture();
    void UpdateRect(const DAVA::Rect& rect);

    void ResetTargetChanged();
    bool IsTargetChanged();

    void ResetLoadedState(bool isLoaded = true);
    bool IsTextureLoaded() const;

    DAVA::Rect GetChangedRect();

    DAVA::int32 GetChangesCount() const;
    void ResetChanges();
    void IncrementChanges();
    void DecrementChanges();

    void UpdateSpriteFromConfig();

    DAVA::NMaterial* GetBrushMaterial() const;

protected:
    DAVA::Texture* customColorsRenderTarget;
    DAVA::Rect changedRect;
    bool spriteChanged;
    bool textureLoaded;
    DAVA::int32 size;

    DAVA::int32 changes;

    DAVA::ScopedPtr<DAVA::NMaterial> brushMaterial;
};

#endif /* defined(__RESOURCEEDITORQT__CUSTOMCOLORSPROXY__) */
