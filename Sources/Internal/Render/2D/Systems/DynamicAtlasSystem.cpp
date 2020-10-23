#include "Render/2D/Systems/DynamicAtlasSystem.h"

#include "Concurrency/LockGuard.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Thread.h"
#include "Logger/Logger.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "Utils/StringFormat.h"
#include "Utils/StringUtils.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Render/Image/ImageSystem.h"
#include "Render/RenderBase.h"
#include "Render/Renderer.h"
#include "Render/Texture.h"
#include "Render/TextureDescriptor.h"
#include "Render/2D/Sprite.h"
#include "Math/RectanglePacker/Spritesheet.h"
#include "Time/SystemTimer.h"

/**
 * Available system options
 *
 * Save atlases as files on every build:
 * #define DEBUG_DUMP_DYNAMIC_ATLASES
 *
 */

namespace DAVA
{
namespace DynamicAtlasSystemDetails
{
/** Return texture descriptor if supported */
std::shared_ptr<TextureDescriptor> GetDescriptorIfValid(const FilePath& textureName);

/** Load first mipmap texture image . */
Image* LoadBaseMipImageForTexture(std::shared_ptr<TextureDescriptor>& texDescriptor);

#ifdef DEBUG_DUMP_DYNAMIC_ATLASES
/** Save image as file for debugging purpose */
void DumpAtlasImage(Image* image);
#endif
}

struct DynamicAtlasSystem::DynamicAtlas
{
    const static uint32 NEED_REBUILD_PERCENT = 50;

    RefPtr<Texture> texture;
    Vector<AtlasRegion*> regions;

    uint32 capacity = 0;
    bool needRebuild = false;

    void RemoveRegion(AtlasRegion* region);
    void AddRegion(AtlasRegion* region);
};

struct DynamicAtlasSystem::AtlasRegion
{
    // Input rectangle packer data
    std::shared_ptr<RectanglePacker::SpriteDefinition> spriteDef;

    // weak pointer
    Sprite* sprite = nullptr;

    Vector<DynamicAtlas*> atlases;
    Vector<std::shared_ptr<TextureDescriptor>> textureDescriptors;

    // Pack results
    Vector<int32> frameTextureIndex;
    Vector<int32> frameNewTextureIndex;
    Vector<Rect2i> frameSheetRects;

    Vector<int32> sheetIndices;
};

void DynamicAtlasSystem::DynamicAtlas::RemoveRegion(AtlasRegion* region)
{
    auto atlasIt = std::find(region->atlases.begin(), region->atlases.end(), this);
    if (atlasIt != region->atlases.end())
    {
        region->atlases.erase(atlasIt);
        auto regionIt = std::find(regions.begin(), regions.end(), region);
        DVASSERT(regionIt != regions.end());
        regions.erase(regionIt);
        needRebuild = (regions.size() * 100) / capacity < NEED_REBUILD_PERCENT;
    }
    else
    {
        DVASSERT(false, "Region not found!");
    }
}

void DynamicAtlasSystem::DynamicAtlas::AddRegion(AtlasRegion* region)
{
    auto atlasIt = std::find(region->atlases.begin(), region->atlases.end(), this);
    if (atlasIt == region->atlases.end())
    {
        DVASSERT(std::find(regions.begin(), regions.end(), region) == regions.end());
        region->atlases.push_back(this);
        regions.push_back(region);
        capacity++;
    }
    else
    {
        // It's ok. Just Skip.
    }
}

DynamicAtlasSystem::DynamicAtlasSystem()
{
    rectanglePacker.SetUseOnlySquareTextures(false);
    rectanglePacker.SetMaxTextureSize(RectanglePacker::DEFAULT_TEXTURE_SIZE);
    rectanglePacker.SetTwoSideMargin(true);
    rectanglePacker.SetTexturesMargin(2);
    // "maxrect_fast"
    rectanglePacker.SetAlgorithms({ PackingAlgorithm::ALG_MAXRECTS_BEST_AREA_FIT });

    Renderer::GetSignals().needRestoreResources.Connect(this, &DynamicAtlasSystem::RebuildAll);
}

DynamicAtlasSystem::~DynamicAtlasSystem()
{
    Renderer::GetSignals().needRestoreResources.Disconnect(this);
    LockGuard<Mutex> lock(systemMutex);
    ReleaseAllAtlases();
    regions.clear();
}

void DynamicAtlasSystem::BeginAtlas(const Vector<String>& whitePathsList_, const Vector<String>& blackPathsList_)
{
    LockGuard<Mutex> lock(systemMutex);
    DVASSERT(linkedThreadId == 0);
    DVASSERT(state == AtlasSystemState::IDLE);
    whitePathsList = whitePathsList_;
    blackPathsList = blackPathsList_;
    state = AtlasSystemState::ACCUMULATING;
    linkedThreadId = Thread::GetCurrentIdAsUInt64();
}

void DynamicAtlasSystem::EndAtlas()
{
    whitePathsList.clear();
    blackPathsList.clear();
    LockGuard<Mutex> lock(systemMutex);
    DVASSERT(state == AtlasSystemState::ACCUMULATING);
    if (needRebuild)
    {
        ReleaseAllAtlases();
        needRebuild = false;
    }
    PackSprites();
    linkedThreadId = 0;
    state = AtlasSystemState::IDLE;
    DVASSERT(unpackedCounter == 0);
}

uint32 DynamicAtlasSystem::GetRegionsCount() const
{
    return static_cast<uint32>(regions.size());
}

uint32 DynamicAtlasSystem::GetAtlasesCount() const
{
    return static_cast<uint32>(atlases.size());
}

bool DynamicAtlasSystem::IsValidPathAndType(const Sprite* sprite) const
{
    auto spritePath = sprite->relativePathname.GetStringValue();
    for (const String& pathPrefix : blackPathsList)
    {
        if (StringUtils::StartsWith(spritePath, pathPrefix))
        {
            return false;
        }
    }

    if (whitePathsList.size() > 0)
    {
        bool inWhiteList = false;
        for (const String& pathPrefix : whitePathsList)
        {
            if (StringUtils::StartsWith(spritePath, pathPrefix))
            {
                inWhiteList = true;
                break;
            }
        }
        if (inWhiteList == false)
        {
            return false;
        }
    }

    return (sprite->type == Sprite::eSpriteType::SPRITE_FROM_FILE);
}

bool DynamicAtlasSystem::RegisterSprite(Sprite* sprite)
{
    if (sprite->inDynamicAtlas == false)
    {
        if (state == ACCUMULATING && IsValidPathAndType(sprite))
        {
            std::shared_ptr<AtlasRegion> region(CreateRegionIfSpriteHasValidFormat(sprite));
            if (nullptr != region)
            {
                LockGuard<Mutex> lock(systemMutex);
                if (state == ACCUMULATING && sprite->inDynamicAtlas == false)
                {
                    DVASSERT(std::find_if(regions.begin(), regions.end(), [&](auto& region) {
                                 return region->sprite == sprite;
                             }) == regions.end());

                    sprite->inDynamicAtlas = true;
                    regions.push_back(region);
                    unpackedCounter++;
                    return true;
                }
            }
        }
    }
    return false;
}

void DynamicAtlasSystem::UnregisterSprite(Sprite* sprite)
{
    if (sprite->inDynamicAtlas)
    {
        LockGuard<Mutex> lock(systemMutex);
        if (sprite->inDynamicAtlas)
        {
            auto regionIt = std::find_if(regions.begin(), regions.end(), [&](auto& region) {
                return region->sprite == sprite;
            });
            if (regionIt != regions.end())
            {
                sprite->inDynamicAtlas = false;
                // release textures
                for (int32 textureIdx = 0; textureIdx < sprite->textureCount; textureIdx++)
                {
                    SafeRelease(sprite->textures[textureIdx]);
                }

                std::shared_ptr<AtlasRegion> region = (*regionIt);
                region->sprite = nullptr;
                regions.erase(regionIt);

                if (region->atlases.size() > 0)
                {
                    RemoveRegionFromAllAtlases(region);
                }
                else
                {
                    unpackedCounter--;
                }
            }
            else
            {
                DVASSERT(false, "Sprite already unregistered!");
            }
        }
    }
}

void DynamicAtlasSystem::RebuildAll()
{
    LockGuard<Mutex> lock(systemMutex);
    ReleaseAllAtlases();
    PackSprites();
}

std::shared_ptr<DynamicAtlasSystem::AtlasRegion> DynamicAtlasSystem::CreateRegionIfSpriteHasValidFormat(Sprite* sprite) const
{
    // Check format and load descriptors
    Vector<std::shared_ptr<TextureDescriptor>> textureDescriptors(sprite->textureCount);
    for (int32 i = 0; i < sprite->textureCount; i++)
    {
        textureDescriptors[i] = DynamicAtlasSystemDetails::GetDescriptorIfValid(sprite->textureNames[i]);
        if (textureDescriptors[i] == nullptr)
        {
            return nullptr;
        }
    }

    auto region = std::make_shared<AtlasRegion>();
    region->sprite = sprite;
    region->textureDescriptors = std::move(textureDescriptors);
    region->frameTextureIndex.resize(sprite->frameCount);

    auto spriteDef = std::make_shared<RectanglePacker::SpriteDefinition>();
    spriteDef->frameRects.resize(sprite->frameCount);

    // Two direction link
    spriteDef->dataPtr = region.get();
    region->spriteDef = spriteDef;

    // Copy frames data for rectangle packer
    for (int32 i = 0; i < sprite->frameCount; i++)
    {
        auto& frameOffests = sprite->rectsAndOffsetsOriginal[i];
        Rect2i frameRect(
        frameOffests[Sprite::eRectsAndOffsets::X_POSITION_IN_TEXTURE],
        frameOffests[Sprite::eRectsAndOffsets::Y_POSITION_IN_TEXTURE],
        frameOffests[Sprite::eRectsAndOffsets::ACTIVE_WIDTH],
        frameOffests[Sprite::eRectsAndOffsets::ACTIVE_HEIGHT]);
        spriteDef->frameRects[i] = frameRect;
        region->frameTextureIndex[i] = sprite->frameTextureIndex[i];
    }
    return region;
}

void DynamicAtlasSystem::PackSprites()
{
    int64 startPackTime = SystemTimer::GetMs();
    Logger::FrameworkDebug("[DynamicAtlasSystem] Start sprites packaging");

    RectanglePacker::PackTask packTask;

    // Collect unpacked sprites list
    for (std::shared_ptr<AtlasRegion>& region : regions)
    {
        if (region->atlases.empty())
        {
            Logger::FrameworkDebug("[DynamicAtlasSystem] Add sprite: %s", region->sprite->relativePathname.GetStringValue().c_str());
            packTask.spriteList.push_back(region->spriteDef);
            unpackedCounter--;
        }
    }
    DVASSERT(unpackedCounter == 0);

    Logger::FrameworkDebug("[DynamicAtlasSystem] Pack sprites: %d from %d", static_cast<int32>(packTask.spriteList.size()), static_cast<int32>(regions.size()));

    // Pack collected sprites to atlases
    if (packTask.spriteList.empty() == false)
    {
        auto startPackAlgoTime = SystemTimer::GetMs();
        std::unique_ptr<RectanglePacker::PackResult> packResult = rectanglePacker.Pack(packTask);
        Logger::FrameworkDebug("[DynamicAtlasSystem] Packing algorithm time: %d", static_cast<int32>(SystemTimer::GetMs() - startPackAlgoTime));
        if (packResult->Success())
        {
            CreateAtlases(packTask, *packResult);
        }
        else
        {
            DVASSERT(false, "Sprites packaging failed!");
        }
    }

    Logger::FrameworkDebug("[DynamicAtlasSystem] Total packaging time: %d", static_cast<int32>(SystemTimer::GetMs() - startPackTime));
    Logger::FrameworkDebug("[DynamicAtlasSystem] App textures count: %d", static_cast<int32>(Texture::GetTextureMap().size()));
}

void DynamicAtlasSystem::CreateAtlases(const RectanglePacker::PackTask& packTask, const RectanglePacker::PackResult& packResult)
{
    Vector<AtlasRegion*> packedRegions;
    packedRegions.reserve(packTask.spriteList.size());

    Logger::FrameworkDebug("[DynamicAtlasSystem] Releasing old textures..");
    // Update sprites
    for (auto& spriteDef : packTask.spriteList)
    {
        AtlasRegion* region = static_cast<AtlasRegion*>(spriteDef->dataPtr);
        // Release old resources
        Sprite* sprite = region->sprite;
        for (int32 textureIdx = 0; textureIdx < sprite->textureCount; textureIdx++)
        {
            SafeRelease(sprite->textures[textureIdx]);
        }
        SafeDeleteArray(sprite->textures);
        sprite->textureCount = 0;
        packedRegions.push_back(region);
    }

    uint32 resultSheetsCount = static_cast<int32>(packResult.resultSheets.size());
    Logger::FrameworkDebug("[DynamicAtlasSystem] Creating %u blank atlas(es)", resultSheetsCount);
    // Create blank images and atlases
    Vector<RefPtr<Image>> finalImages(resultSheetsCount);
    Vector<std::shared_ptr<DynamicAtlas>> finalAtlases(resultSheetsCount);

    for (uint32 i = 0; i < resultSheetsCount; ++i)
    {
        const Rect2i rect = packResult.resultSheets[i]->GetRect();
        RefPtr<Image> image(Image::Create(rect.dx, rect.dy, PixelFormat::FORMAT_RGBA8888));
        Memset(image->data, 0, image->dataSize);
        finalImages[i] = image;

        auto atlas = std::make_shared<DynamicAtlas>();
        finalAtlases[i] = atlas;
        atlases.push_back(atlas);
    }

    Logger::FrameworkDebug("[DynamicAtlasSystem] Frames composing..");
    // Combine final images from sprites
    for (const RectanglePacker::SpriteIndexedData& spriteData : packResult.resultIndexedSprites)
    {
        RectanglePacker::SpriteDefinition* spriteDef = spriteData.spriteDef.get();
        AtlasRegion* region = static_cast<AtlasRegion*>(spriteDef->dataPtr);
        Vector<int32> sheetIndexToTextureIndexTable(finalImages.size(), -1);

        Vector<RefPtr<Image>> images;
        uint32 frameCount = spriteDef->GetFrameCount();
        region->frameNewTextureIndex.resize(frameCount);
        region->frameSheetRects.resize(frameCount);
        region->sheetIndices.clear();

        // Load sprite images
        for (std::shared_ptr<TextureDescriptor>& textureDescriptor : region->textureDescriptors)
        {
            RefPtr<Image> image(DynamicAtlasSystemDetails::LoadBaseMipImageForTexture(textureDescriptor));
            // Image can be nullptr. It will check later.
            images.push_back(image);
        }

        // Combine sprite frames into atlas
        for (uint32 frameIdx = 0; frameIdx < frameCount; frameIdx++)
        {
            const SpriteBoundsRect* packedInfo = spriteData.frameToPackedInfo[frameIdx];
            int32 sheetIndex = spriteData.frameToSheetIndex[frameIdx];
            DVASSERT(-1 < sheetIndex && sheetIndex < static_cast<int32>(packResult.resultSheets.size()));
            DVASSERT(packedInfo);
            // Build sprite textures index
            DVASSERT(sheetIndex < static_cast<int32>(sheetIndexToTextureIndexTable.size()));
            int32 newTextureIndex = sheetIndexToTextureIndexTable[sheetIndex];
            if (newTextureIndex == -1)
            {
                newTextureIndex = static_cast<int32>(region->sheetIndices.size());
                sheetIndexToTextureIndexTable[sheetIndex] = newTextureIndex;
                region->sheetIndices.push_back(sheetIndex);
            }
            region->frameNewTextureIndex[frameIdx] = newTextureIndex;

            // Prepare geometric informations
            const Rect2i& spriteRect = packedInfo->spriteRect;
            region->frameSheetRects[frameIdx] = spriteRect;
            finalAtlases[sheetIndex]->AddRegion(region);

            RefPtr<Image> finalImage = finalImages[sheetIndex];
            Rect2i& frameRect = spriteDef->frameRects[frameIdx];
            RefPtr<Image> image = images[region->frameTextureIndex[frameIdx]];

            // Render frames into atlas
            if (image != nullptr)
            {
                DVASSERT(image->GetPixelFormat() == FORMAT_RGBA8888);
                finalImage->InsertImage(image.Get(), spriteRect.x, spriteRect.y, frameRect.x, frameRect.y, frameRect.dx, frameRect.dy);
            }
            else
            {
                image = RefPtr<Image>(Image::Create(frameRect.dx, frameRect.dy, FORMAT_RGBA8888));
                image->MakePink(false);
                finalImage->InsertImage(image.Get(), spriteRect.x, spriteRect.y, 0, 0, frameRect.dx, frameRect.dy);
            }
        }

        DVASSERT(region->atlases.size() > 0);
    }

    Logger::FrameworkDebug("[DynamicAtlasSystem] Textures creating..");
    // Create textures and release in memory images
    for (uint32 imageNum = 0; imageNum < finalImages.size(); ++imageNum)
    {
#ifdef DEBUG_DUMP_DYNAMIC_ATLASES
        DynamicAtlasSystemDetails::DumpAtlasImage(finalImages[imageNum].Get());
#endif
        // Create texture from image
        RefPtr<Texture> atlasTexture(Texture::CreateFromData({ finalImages[imageNum].Get() }));
        atlasTexture->texDescriptor->pathname = Format("memoryfile_dynamic_atlas_%d", atlasCounter);
        atlasCounter++;
        finalAtlases[imageNum]->texture = atlasTexture;
        finalImages[imageNum] = nullptr;
    }
    finalImages.clear();

    Logger::FrameworkDebug("[DynamicAtlasSystem] Geometry updating..");
    // Update sprites
    for (AtlasRegion* region : packedRegions)
    {
        // Update textures
        Sprite* sprite = region->sprite;
        const int32 textureCount = static_cast<int32>(region->sheetIndices.size());
        DVASSERT(textureCount > 0);
        DVASSERT(sprite->textures == nullptr);
        sprite->textureCount = textureCount;
        sprite->textures = new Texture*[textureCount];

        // Fill frame textures
        for (int32 textureIdx = 0; textureIdx < textureCount; textureIdx++)
        {
            uint32 sheetIndex = region->sheetIndices[textureIdx];
            DVASSERT(sheetIndex < finalAtlases.size());
            std::shared_ptr<DynamicAtlas>& atlas = finalAtlases[sheetIndex];
            Texture* texture = atlas->texture.Get();
            sprite->textures[textureIdx] = SafeRetain(texture);
        }
        // Update frames geometry
        for (int32 frameIdx = 0; frameIdx < sprite->frameCount; frameIdx++)
        {
            sprite->frameTextureIndex[frameIdx] = region->frameNewTextureIndex[frameIdx];
            auto& frameRectPacked = region->frameSheetRects[frameIdx];
            sprite->UpdateFrameGeometry(frameRectPacked.x, frameRectPacked.y, frameIdx);
        }
    }
    Logger::FrameworkDebug("[DynamicAtlasSystem] Atlases ready");
}

void DynamicAtlasSystem::ReleaseAllAtlases()
{
    Vector<Sprite*> sprites;
    for (std::shared_ptr<DynamicAtlas>& atlas : atlases)
    {
        for (AtlasRegion* region : atlas->regions)
        {
            Sprite* sprite = region->sprite;
            DVASSERT(sprite);
            for (int32 textureIdx = 0; textureIdx < sprite->textureCount; textureIdx++)
            {
                SafeRelease(sprite->textures[textureIdx]);
            }
            region->atlases.clear();
        }
        atlas->regions.clear();
        DVASSERT(atlas->texture->GetRetainCount() == 1);
    }
    atlases.clear();
    unpackedCounter = static_cast<uint32>(regions.size());
}

void DynamicAtlasSystem::RemoveRegionFromAllAtlases(std::shared_ptr<AtlasRegion>& region)
{
    // Remove from parent atlases
    Vector<DynamicAtlas*> atlasesCopy(region->atlases);
    for (DynamicAtlas* atlas : atlasesCopy)
    {
        if (atlas != nullptr)
        {
            atlas->RemoveRegion(region.get());

            if (atlas->regions.empty())
            {
                // Remove atlas if empty
                DVASSERT(atlas->texture->GetRetainCount() == 1);
                atlas->texture = nullptr;
                auto ait = std::find_if(atlases.begin(), atlases.end(), [&](auto& atlasPtr) { return atlasPtr.get() == atlas; });
                DVASSERT(ait != atlases.end());
                atlases.erase(ait);
            }
            else
            {
                needRebuild |= atlas->needRebuild;
            }
        }
    }
}

void DynamicAtlasSystem::Clear()
{
    LockGuard<Mutex> lock(systemMutex);
    DVASSERT(state == IDLE);
    ReleaseAllAtlases();
    for (std::shared_ptr<AtlasRegion>& region : regions)
    {
        Sprite* sprite = region->sprite;
        // Prevent "DynamicAtlasSystem::UnregisterSprite()" call
        sprite->inDynamicAtlas = false;
        sprite->Reload();
        DVASSERT(false == sprite->inDynamicAtlas);
    }
    regions.clear();
    unpackedCounter = 0;
}

namespace DynamicAtlasSystemDetails
{
std::shared_ptr<TextureDescriptor> GetDescriptorIfValid(const FilePath& textureName)
{
    std::shared_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(TextureDescriptor::GetDescriptorPathname(textureName)));

    if (descriptor == nullptr || descriptor->IsCubeMap())
    {
        return nullptr;
    }
    if (descriptor->isCompressedFile)
    {
        if (descriptor->format == FORMAT_RGBA8888 || descriptor->format == FORMAT_RGB888)
        {
            return descriptor;
        }
    }
    else
    {
        for (eGPUFamily gpu : Texture::GetGPULoadingOrder())
        {
            PixelFormat format = descriptor->GetPixelFormatForGPU(gpu);
            if (format == FORMAT_RGBA8888 || format == FORMAT_RGB888)
            {
                return descriptor;
            }
        }
    }
    return nullptr;
}

Image* LoadBaseMipImageForTexture(std::shared_ptr<TextureDescriptor>& texDescriptor)
{
    if (nullptr == texDescriptor || texDescriptor->IsCubeMap())
    {
        return nullptr;
    }

    Image* image = nullptr;
    for (eGPUFamily gpu : Texture::GetGPULoadingOrder())
    {
        eGPUFamily gpuForLoading = Texture::GetGPUForLoading(gpu, texDescriptor.get());

        FilePath multipleMipPathname = texDescriptor->CreateMultiMipPathnameForGPU(gpuForLoading);
        image = ImageSystem::LoadSingleMip(multipleMipPathname, 0);
        if (image != nullptr)
        {
            if (image->format != FORMAT_RGBA8888)
            {
                Image* newImage = Image::Create(image->width, image->height, FORMAT_RGBA8888);
                bool converted = ImageConvert::ConvertImage(image, newImage);
                if (converted)
                {
                    SafeRelease(image);
                    image = newImage;
                }
                else
                {
                    SafeRelease(newImage);
                    SafeRelease(image);
                    image = nullptr;
                }
            }
            break;
        }
    }

    return image;
}

#ifdef DEBUG_DUMP_DYNAMIC_ATLASES

void DumpAtlasImage(Image* image)
{
    // Save atlas image as file
    int64 startImageSaveTime = SystemTimer::GetMs();
    static int32 atalasId = 0;
    FilePath path = Format("~doc:/atlas_%03d.png", atalasId++);
    bool saved = image->Save(path);
    Logger::FrameworkDebug("[DynamicAtlasSystem] Atlas image dump: \"%s\" time: %d", path.GetStringValue().c_str(), static_cast<int32>(SystemTimer::GetMs() - startImageSaveTime));
}

#endif
}
};
