#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Concurrency/Mutex.h"
#include "Math/RectanglePacker/RectanglePacker.h"

namespace DAVA
{
class Sprite;
class TextureDescriptor;
class Image;
class Texture;

class DefinitionFile;
struct SpritesheetLayout;

/**
 * System for baking many sprites into large texture atlases at runtime.
 * Supports only images with uncompressed format (RGB888, RGBA8888).
 * Adds one pixel margin between regions to avoid artifacts. 
 * Automatically dispose atlas textures when all related sprites deleted.
 * Call "Sprites::Reload()" removes sprite from atlas and load defaults.
 * 
 * Attention!!! 
 * You must not render 'isDynamicAtals' sprites loaded between 'BeginAtlas' and 'EndAtlas' calls, because textures will be null.
 * Not recommended to using for sprites with shared textures.
 */
class DynamicAtlasSystem final
{
public:
    DynamicAtlasSystem();
    ~DynamicAtlasSystem();

    /** Remove all atlases and load default sprites */
    void Clear();
    /** 
     * Start accumulating new sprites. 
     * \param whitePathsList - if param not empty system will register only sprites from white paths.
     * \param blackPathsList - if param not empty system will ignore sprites from black paths. BlackPathsList is more priority than whitePathsList.
    */
    void BeginAtlas(const Vector<String>& whitePathsList, const Vector<String>& blackPathsList);
    /** 
     * Stop accumulation sprites and pack atlases. 
     * Automatically repacks existing atlases when "size / capacity <= 0.5" 
     */
    void EndAtlas();

    /** Remove existing atlases and repack all regions */
    void RebuildAll();

    uint32 GetRegionsCount() const;
    uint32 GetAtlasesCount() const;

private:
    struct AtlasRegion;
    struct DynamicAtlas;

    enum AtlasSystemState
    {
        ACCUMULATING,
        BUILD,
        IDLE
    };

    /** Try add sprite to system. Returns 'true' if success. */
    bool RegisterSprite(Sprite* sprite);
    /** 
     * Remove sprite from system. 
     * Automatically dispose atlas textures when all related sprites deleted.
     */
    void UnregisterSprite(Sprite* sprite);

    /** Create and fill region for sprite */
    std::shared_ptr<DynamicAtlasSystem::AtlasRegion> CreateRegionIfSpriteHasValidFormat(Sprite* sprite) const;

    /** Fast check sprite. */
    bool IsValidPathAndType(const Sprite* sprite) const;

    /** Pack all unpacked sprites */
    void PackSprites();

    /** Create atlases and update sprite textures */
    void CreateAtlases(const RectanglePacker::PackTask& packTask, const RectanglePacker::PackResult& packResult);

    /** Remove all atlases and release related resources*/
    void ReleaseAllAtlases();

    /** Remove region form related atlases and remove empty atlases. */
    void RemoveRegionFromAllAtlases(std::shared_ptr<AtlasRegion>& region);

    volatile AtlasSystemState state = AtlasSystemState::IDLE;
    Vector<std::shared_ptr<DynamicAtlas>> atlases;
    Vector<std::shared_ptr<AtlasRegion>> regions;

    Vector<String> whitePathsList;
    Vector<String> blackPathsList;

    int32 unpackedCounter = 0;
    bool needRebuild = false;

    uint64 linkedThreadId = 0; // Identifier of thread that calls DynamicAtlasSystem::BeginAtlas() method
    Mutex systemMutex;
    int32 atlasCounter = 0;

    RectanglePacker rectanglePacker;

    friend class Sprite;
};
}
