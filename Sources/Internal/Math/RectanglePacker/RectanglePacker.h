#pragma once 

#include "Math/RectanglePacker/Spritesheet.h"
#include "Functional/Function.h"
#include "Base/BaseTypes.h"
#include "Math/Math2D.h"

namespace DAVA
{
class RectanglePacker final
{
public:
    struct SpriteDefinition
    {
        /** Original rects for packing */
        Vector<Rect2i> frameRects;
        /** Linked user data for  sprite */
        void* dataPtr = nullptr;

        uint32 GetFrameCount() const;
        Size2i GetFrameSize(uint32 frame) const;
        int32 GetFrameWidth(uint32 frame) const;
        int32 GetFrameHeight(uint32 frame) const;
    };

    struct SpriteIndexedData
    {
        std::shared_ptr<SpriteDefinition> spriteDef;
        Vector<DAVA::int32> frameToSheetIndex;
        Vector<const SpriteBoundsRect*> frameToPackedInfo;
    };

    struct PackTask
    {
        /** Input sprites data. Shared pointer required for using frameRects pointers as keys. */
        Vector<std::shared_ptr<SpriteDefinition>> spriteList;
        /** Pack params */
        bool needSquareTextureOverriden = false;
    };
    struct PackResult
    {
        /** Result spritesheets */
        Vector<std::unique_ptr<SpritesheetLayout>> resultSheets;
        /** Packaging errors */
        Set<String> resultErrors;
        /** Indexed sprites data for fast processing */
        Vector<RectanglePacker::SpriteIndexedData> resultIndexedSprites;
        bool Success() const;
    };

    struct SpriteItem
    {
        std::shared_ptr<SpriteDefinition> defFile;
        uint32 spriteWeight = 0;
        uint32 frameIndex = 0;
    };

    static const uint32 DEFAULT_TEXTURE_SIZE = 2048;
    static const uint32 DEFAULT_MARGIN = 1;

    RectanglePacker();

    void SetUseOnlySquareTextures(bool value);
    void SetMaxTextureSize(uint32 maxTextureSize);
    void SetAlgorithms(const Vector<PackingAlgorithm>& algorithms);
    // set visible 1 pixel border for each texture
    void SetTwoSideMargin(bool val = true);
    void SetTexturesMargin(uint32 margin);

    /** Pack sprites from packTask and return PackResult with spritesheets data */
    std::unique_ptr<PackResult> Pack(PackTask& packTask) const;

private:
    std::unique_ptr<PackResult> PackSprites(Vector<SpriteItem>& spritesToPack, PackTask& packTask) const;
    uint32 TryToPack(SpritesheetLayout* sheet, Vector<SpriteItem>& tempSortVector, bool fullPackOnly) const;
    void CreateSpritesIndex(RectanglePacker::PackTask& packTask, RectanglePacker::PackResult* packResult) const;

    Vector<PackingAlgorithm> packAlgorithms;
    uint32 maxTextureSize = DEFAULT_TEXTURE_SIZE;
    bool onlySquareTextures = false;
    bool useTwoSideMargin = false;
    uint32 texturesMargin = 1;
};

inline void RectanglePacker::SetUseOnlySquareTextures(bool value)
{
    onlySquareTextures = value;
}

inline void RectanglePacker::SetMaxTextureSize(uint32 _maxTextureSize)
{
    maxTextureSize = _maxTextureSize;
}

inline void RectanglePacker::SetTwoSideMargin(bool val)
{
    useTwoSideMargin = val;
}
inline void RectanglePacker::SetTexturesMargin(uint32 margin)
{
    texturesMargin = margin;
}

inline void RectanglePacker::SetAlgorithms(const Vector<PackingAlgorithm>& algorithms)
{
    packAlgorithms = algorithms;
}
inline uint32 RectanglePacker::SpriteDefinition::GetFrameCount() const
{
    return static_cast<uint32>(frameRects.size());
}

inline Size2i RectanglePacker::SpriteDefinition::GetFrameSize(uint32 frame) const
{
    return Size2i(frameRects[frame].dx, frameRects[frame].dy);
}

inline int RectanglePacker::SpriteDefinition::GetFrameWidth(uint32 frame) const
{
    return frameRects[frame].dx;
}

inline int RectanglePacker::SpriteDefinition::GetFrameHeight(uint32 frame) const
{
    return frameRects[frame].dy;
}

inline bool RectanglePacker::PackResult::Success() const
{
    return resultErrors.empty();
}
};
