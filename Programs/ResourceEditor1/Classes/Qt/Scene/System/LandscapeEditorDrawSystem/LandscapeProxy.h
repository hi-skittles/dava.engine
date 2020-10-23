#pragma once

#include "DAVAEngine.h"

#include "Render/UniqueStateSet.h"

namespace DAVA
{
class Image;
}

class LandscapeProxy : public DAVA::BaseObject
{
public:
    enum eTilemaskTextures
    {
        TILEMASK_TEXTURE_SOURCE = 0,
        TILEMASK_TEXTURE_DESTINATION,

        TILEMASK_TEXTURE_COUNT
    };

    enum eLandscapeMode
    {
        MODE_CUSTOM_LANDSCAPE = 0,
        MODE_ORIGINAL_LANDSCAPE,

        MODES_COUNT
    };

    static const DAVA::FastName LANDSCAPE_TEXTURE_TOOL;
    static const DAVA::FastName LANDSCAPE_TEXTURE_CURSOR; //should use clamp wrap mode
    static const DAVA::FastName LANSDCAPE_FLAG_CURSOR;
    static const DAVA::FastName LANSDCAPE_FLAG_TOOL;
    static const DAVA::FastName LANSDCAPE_FLAG_TOOL_MIX;
    static const DAVA::FastName LANDSCAPE_PARAM_CURSOR_COORD_SIZE; //x,y - cursor position [0...1] (in landscape space); z,w - cursor size [0...1] (fraction of landscape)

protected:
    virtual ~LandscapeProxy();

public:
    LandscapeProxy(DAVA::Landscape* landscape, DAVA::Entity* node);

    void SetMode(LandscapeProxy::eLandscapeMode mode);

    const DAVA::AABBox3& GetLandscapeBoundingBox();
    DAVA::Texture* GetLandscapeTexture(const DAVA::FastName& level);
    DAVA::Color GetLandscapeTileColor(const DAVA::FastName& level);
    void SetLandscapeTileColor(const DAVA::FastName& level, const DAVA::Color& color);

    void SetToolTexture(DAVA::Texture* texture, bool mixColors);

    void SetHeightmap(DAVA::Heightmap* heightmap);

    void CursorEnable();
    void CursorDisable();
    void SetCursorTexture(DAVA::Texture* texture);
    void SetCursorSize(DAVA::float32 size);
    void SetCursorPosition(const DAVA::Vector2& position);

    DAVA::Vector3 PlacePoint(const DAVA::Vector3& point);

    bool IsTilemaskChanged();
    void ResetTilemaskChanged();
    void IncreaseTilemaskChanges();
    void DecreaseTilemaskChanges();

    bool InitTilemaskImageCopy(const DAVA::FilePath& sourceTilemaskPath);
    DAVA::Image* GetTilemaskImageCopy();

    void InitTilemaskDrawTextures();
    DAVA::Texture* GetTilemaskDrawTexture(DAVA::int32 number);
    void SwapTilemaskDrawTextures();

private:
    void RestoreResources();

protected:
    enum eToolTextureType
    {
        TEXTURE_TYPE_NOT_PASSABLE = 0,
        TEXTURE_TYPE_CUSTOM_COLORS,
        TEXTURE_TYPE_VISIBILITY_CHECK_TOOL,
        TEXTURE_TYPE_RULER_TOOL,

        TEXTURE_TYPES_COUNT
    };

    DAVA::Image* tilemaskImageCopy = nullptr;
    DAVA::Array<DAVA::Texture*, TILEMASK_TEXTURE_COUNT> tilemaskDrawTextures;

    DAVA::int32 tilemaskWasChanged = 0;

    DAVA::Landscape* baseLandscape = nullptr;
    DAVA::NMaterial* landscapeEditorMaterial = nullptr;
    DAVA::Vector4 cursorCoordSize;

    eLandscapeMode mode = MODE_ORIGINAL_LANDSCAPE;

    DAVA::Texture* cursorTexture = nullptr;
};
