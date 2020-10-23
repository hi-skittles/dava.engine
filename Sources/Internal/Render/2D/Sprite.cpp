#include "Render/2D/Sprite.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "FileSystem/File.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/LocalizationSystem.h"
#include "FileSystem/UnmanagedMemoryFile.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/DynamicAtlasSystem.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Render/Image/ImageSystem.h"
#include "Render/RenderHelper.h"
#include "Render/Shader.h"
#include "Render/TextureDescriptor.h"
#include "Time/SystemTimer.h"
#include "UI/UIControlSystem.h"
#include "Utils/StringFormat.h"
#include "Utils/Utils.h"
#include "Engine/EngineContext.h"

#define NEW_PPA

namespace DAVA
{
#ifdef USE_FILEPATH_IN_MAP
using SpriteMap = Map<FilePath, Sprite*>;
#else //#ifdef USE_FILEPATH_IN_MAP
using SpriteMap = Map<String, Sprite*>;
#endif //#ifdef USE_FILEPATH_IN_MAP
SpriteMap spriteMap;

static int32 fboCounter = 0;

Mutex Sprite::spriteMapMutex;

SpriteDrawState::SpriteDrawState()
{
    Reset();

    material = RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL;
}

Sprite::Sprite()
{
    textures = 0;
    textureNames = 0;
    frameTextureIndex = 0;
    textureCount = 0;

    frameVertices = 0;
    texCoords = 0;
    rectsAndOffsets = 0;

    size.dx = 24;
    size.dy = 24;
    frameCount = 0;

    isPreparedForTiling = false;
    textureInVirtualSpace = false;

    modification = 0;
    flags = 0;
    resourceSizeIndex = 0;

    clipPolygon = 0;

    defaultPivotPoint = Vector2(0.0f, 0.0f);

    type = SPRITE_FROM_FILE;
}

Sprite* Sprite::PureCreate(const FilePath& spriteName, Sprite* forPointer)
{
    if (spriteName.IsEmpty() || spriteName.GetType() == FilePath::PATH_IN_MEMORY)
        return NULL;

    Sprite* cachedSprite = GetSpriteFromMap(spriteName);
    if (cachedSprite)
    {
        return cachedSprite;
    }

    int32 resourceSizeIndex = 0;
    File* spriteFile = GetSpriteFile(spriteName, resourceSizeIndex);
    if (!spriteFile)
    {
        return NULL;
    }

    Sprite* spr = forPointer;
    if (!spr)
    {
        spr = new Sprite();
    }

    spr->resourceSizeIndex = resourceSizeIndex;
    spr->relativePathname = spriteName;

    spr->InitFromFile(spriteFile);
    SafeRelease(spriteFile);

    spriteMapMutex.Lock();
    spriteMap[FILEPATH_MAP_KEY(spr->relativePathname)] = spr;
    spriteMapMutex.Unlock();

    spr->Reset();
    return spr;
}

Sprite* Sprite::GetSpriteFromMap(const FilePath& pathname)
{
    Sprite* ret = NULL;

    FilePath path = pathname;

    spriteMapMutex.Lock();

    SpriteMap::iterator it = spriteMap.find(FILEPATH_MAP_KEY(path));
    if (it != spriteMap.end())
    {
        Sprite* spr = it->second;
        spr->Retain();
        ret = spr;
    }
    spriteMapMutex.Unlock();

    return ret;
}

FilePath Sprite::GetScaledName(const FilePath& spriteName)
{
    String pathname;
    if (FilePath::PATH_IN_RESOURCES == spriteName.GetType())
        pathname = spriteName.GetFrameworkPath(); //as we can have several res folders we should work with 'FrameworkPath' instead of 'AbsolutePathname'
    else
        pathname = spriteName.GetAbsolutePathname();

    VirtualCoordinatesSystem* virtualCoordsSystem = GetEngineContext()->uiControlSystem->vcs;
    const String baseGfxFolderName = virtualCoordsSystem->GetResourceFolder(virtualCoordsSystem->GetBaseResourceIndex());
    String::size_type pos = pathname.find(baseGfxFolderName);
    if (String::npos != pos)
    {
        const String& desirableGfxFolderName = virtualCoordsSystem->GetResourceFolder(virtualCoordsSystem->GetDesirableResourceIndex());
        pathname.replace(pos, baseGfxFolderName.length(), desirableGfxFolderName);
        return pathname;
    }
    else if (virtualCoordsSystem->GetResourceFoldersCount() == 1 && baseGfxFolderName != "Gfx")
    { // magic for QE

        String::size_type startPos = pathname.find("/Gfx/");
        if (String::npos != startPos)
        {
            pathname.replace(startPos, 5, "/" + baseGfxFolderName + "/");
            return pathname;
        }
    }

    return spriteName;
}

File* Sprite::LoadLocalizedFile(const FilePath& spritePathname, FilePath& texturePath)
{
    FilePath localizedScaledPath(spritePathname);
    localizedScaledPath.ReplaceDirectory(spritePathname.GetDirectory() + (GetEngineContext()->localizationSystem->GetCurrentLocale() + "/"));

    texturePath = FilePath();
    File* fp = File::Create(localizedScaledPath, File::READ | File::OPEN);
    if (fp)
    {
        texturePath = localizedScaledPath;
    }
    else
    {
        fp = File::Create(spritePathname, File::READ | File::OPEN);
        if (fp)
        {
            texturePath = spritePathname;
        }
    }

    return fp;
}

void Sprite::InitFromFile(File* file)
{
    type = SPRITE_FROM_FILE;
    const FilePath& pathName = file->GetFilename();

    char tempBuf[1024];
    file->ReadLine(tempBuf, 1024);
    sscanf(tempBuf, "%d", &textureCount);
    textures = new Texture*[textureCount];
    textureNames = new FilePath[textureCount];

    char textureCharName[128];
    for (int32 k = 0; k < textureCount; ++k)
    {
        file->ReadLine(tempBuf, 1024);
        sscanf(tempBuf, "%s", textureCharName);

        FilePath tp = pathName.GetDirectory() + String(textureCharName);
        textureNames[k] = tp;
        textures[k] = nullptr;
    }

    int32 width, height;
    file->ReadLine(tempBuf, 1024);
    sscanf(tempBuf, "%d %d", &width, &height);
    size = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtual(Vector2(float32(width), float32(height)), resourceSizeIndex);

    file->ReadLine(tempBuf, 1024);
    sscanf(tempBuf, "%d", &frameCount);

    texCoords = new float32*[frameCount];
    frameVertices = new float32*[frameCount];
    rectsAndOffsets = new float32*[frameCount];
    frameTextureIndex = new int32[frameCount];
    rectsAndOffsetsOriginal.resize(frameCount);

    frameNames.resize(frameCount);
    for (int32 i = 0; i < frameCount; i++)
    {
        frameVertices[i] = new float32[8];
        texCoords[i] = new float32[8];
        rectsAndOffsets[i] = new float32[6];
        char frameName[128] = { 0 };

        int32 x, y, dx, dy, xOff, yOff;

        file->ReadLine(tempBuf, 1024);
        sscanf(tempBuf, "%d %d %d %d %d %d %d %s", &x, &y, &dx, &dy, &xOff, &yOff, &frameTextureIndex[i], frameName);
        frameNames[i] = (*frameName == '\0') ? FastName() : FastName(frameName);

        rectsAndOffsetsOriginal[i][eRectsAndOffsets::X_POSITION_IN_TEXTURE] = x;
        rectsAndOffsetsOriginal[i][eRectsAndOffsets::Y_POSITION_IN_TEXTURE] = y;
        rectsAndOffsetsOriginal[i][eRectsAndOffsets::ACTIVE_WIDTH] = dx;
        rectsAndOffsetsOriginal[i][eRectsAndOffsets::ACTIVE_HEIGHT] = dy;
        rectsAndOffsetsOriginal[i][eRectsAndOffsets::X_OFFSET_TO_ACTIVE] = xOff;
        rectsAndOffsetsOriginal[i][eRectsAndOffsets::Y_OFFSET_TO_ACTIVE] = yOff;
    }
    defaultPivotPoint.x = 0;
    defaultPivotPoint.y = 0;

    if (GetEngineContext()->dynamicAtlasSystem->RegisterSprite(this))
    {
        // Sprite was added into system
        // Texture will created in `DynamicAtlasSystem::EndAtlas()` call.
    }
    else
    {
        // Load default textures
        for (int32 k = 0; k < textureCount; ++k)
        {
            textures[k] = Texture::CreateFromFile(textureNames[k]);
            DVASSERT(textures[k], "ERROR: Texture loading failed" /* + pathName*/);
        }
    }

    // Update default geometry
    for (int32 i = 0; i < frameCount; i++)
    {
        UpdateFrameGeometry(rectsAndOffsetsOriginal[i][eRectsAndOffsets::X_POSITION_IN_TEXTURE],
                            rectsAndOffsetsOriginal[i][eRectsAndOffsets::Y_POSITION_IN_TEXTURE], i);
    }

    if (!inDynamicAtlas)
    {
        // Reduces memory usage by freeing unused memory
        rectsAndOffsetsOriginal.clear();
        rectsAndOffsetsOriginal.shrink_to_fit();
    }
}

void Sprite::UpdateFrameGeometry(int32 x, int32 y, int32 frameIdx)
{
    DVASSERT(rectsAndOffsetsOriginal.size() == frameCount);

    float32 dx = static_cast<float32>(rectsAndOffsetsOriginal[frameIdx][eRectsAndOffsets::ACTIVE_WIDTH]);
    float32 dy = static_cast<float32>(rectsAndOffsetsOriginal[frameIdx][eRectsAndOffsets::ACTIVE_HEIGHT]);
    float32 xOff = static_cast<float32>(rectsAndOffsetsOriginal[frameIdx][eRectsAndOffsets::X_OFFSET_TO_ACTIVE]);
    float32 yOff = static_cast<float32>(rectsAndOffsetsOriginal[frameIdx][eRectsAndOffsets::Y_OFFSET_TO_ACTIVE]);

    Rect rect = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtual(Rect(float32(xOff), float32(yOff), float32(dx), float32(dy)), resourceSizeIndex);

    rectsAndOffsets[frameIdx][eRectsAndOffsets::X_POSITION_IN_TEXTURE] = static_cast<float32>(x);
    rectsAndOffsets[frameIdx][eRectsAndOffsets::Y_POSITION_IN_TEXTURE] = static_cast<float32>(y);
    rectsAndOffsets[frameIdx][eRectsAndOffsets::ACTIVE_WIDTH] = rect.dx;
    rectsAndOffsets[frameIdx][eRectsAndOffsets::ACTIVE_HEIGHT] = rect.dy;
    rectsAndOffsets[frameIdx][eRectsAndOffsets::X_OFFSET_TO_ACTIVE] = rect.x;
    rectsAndOffsets[frameIdx][eRectsAndOffsets::Y_OFFSET_TO_ACTIVE] = rect.y;

    frameVertices[frameIdx][0] = rect.x;
    frameVertices[frameIdx][1] = rect.y;
    frameVertices[frameIdx][2] = rect.x + rect.dx;
    frameVertices[frameIdx][3] = rect.y;
    frameVertices[frameIdx][4] = rect.x;
    frameVertices[frameIdx][5] = rect.y + rect.dy;
    frameVertices[frameIdx][6] = rect.x + rect.dx;
    frameVertices[frameIdx][7] = rect.y + rect.dy;

    dx += x;
    dy += y;

    Texture* frameTexture = textures[frameTextureIndex[frameIdx]];
    float32 texWidth = 1.0;
    float32 texHeight = 1.0;
    if (frameTexture != nullptr)
    {
        texWidth = static_cast<float32>(frameTexture->width);
        texHeight = static_cast<float32>(frameTexture->height);
    }

    float32 textureX = x / texWidth;
    float32 textureDX = dx / texWidth;
    float32 textureY = y / texHeight;
    float32 textureDY = dy / texHeight;
    texCoords[frameIdx][0] = textureX;
    texCoords[frameIdx][1] = textureY;
    texCoords[frameIdx][2] = textureDX;
    texCoords[frameIdx][3] = textureY;
    texCoords[frameIdx][4] = textureX;
    texCoords[frameIdx][5] = textureDY;
    texCoords[frameIdx][6] = textureDX;
    texCoords[frameIdx][7] = textureDY;
}

Sprite* Sprite::Create(const FilePath& spriteName)
{
    String extension = spriteName.GetExtension();

    Sprite* spr = nullptr;
    if (!extension.empty() && TextureDescriptor::IsSourceTextureExtension(extension))
    {
        spr = CreateFromSourceFile(spriteName);
    }
    else
    {
        spr = PureCreate(spriteName, NULL);
    }

    if (!spr)
    {
        Texture* pinkTexture = Texture::CreatePink();
        spr = CreateFromTexture(pinkTexture, 0, 0, 16, 16, 16.f, 16.f, spriteName);
        spr->type = SPRITE_FROM_FILE;

        pinkTexture->Release();
    }
    return spr;
}

Sprite* Sprite::CreateFromTexture(Texture* fromTexture, int32 xOffset, int32 yOffset, float32 sprWidth, float32 sprHeight, bool contentScaleIncluded)
{
    DVASSERT(fromTexture);
    Sprite* spr = new Sprite();
    DVASSERT(spr, "Render Target Sprite Creation failed");
    spr->InitFromTexture(fromTexture, xOffset, yOffset, sprWidth, sprHeight, -1, -1, contentScaleIncluded);
    return spr;
}

Sprite* Sprite::CreateFromTexture(Texture* fromTexture, int32 textureRegionOffsetX, int32 textureRegionOffsetY, int32 textureRegionWidth, int32 textureRegionHeigth, float32 sprWidth, float32 sprHeight, const FilePath& spriteName /* = FilePath()*/)
{
    DVASSERT(fromTexture);
    Sprite* spr = new Sprite();
    DVASSERT(spr, "Render Target Sprite Creation failed");
    spr->InitFromTexture(fromTexture, textureRegionOffsetX, textureRegionOffsetY, sprWidth, sprHeight, textureRegionWidth, textureRegionHeigth, false, spriteName);
    return spr;
}

Sprite* Sprite::CreateFromImage(Image* image, bool contentScaleIncluded /* = false*/, bool inVirtualSpace /* = false */)
{
    uint32 width = image->GetWidth();
    uint32 height = image->GetHeight();

    ScopedPtr<Image> squareImage(ImageSystem::EnsurePowerOf2Image(image));
    ScopedPtr<Texture> texture(Texture::CreateFromData(squareImage, false));

    Sprite* sprite = nullptr;
    if (texture)
    {
        Vector2 sprSize((float32(width)), (float32(height)));
        if (inVirtualSpace)
        {
            sprSize = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtual(sprSize);
        }

        sprite = Sprite::CreateFromTexture(texture, 0, 0, sprSize.x, sprSize.y, contentScaleIncluded);

        if (inVirtualSpace)
        {
            sprite->ConvertToVirtualSize();
        }
    }

    return sprite;
}

Sprite* Sprite::CreateFromSourceData(const uint8* data, uint32 size, bool contentScaleIncluded /* = false*/, bool inVirtualSpace /* = false */)
{
    if (data == nullptr || size == 0)
    {
        return nullptr;
    }

    ScopedPtr<UnmanagedMemoryFile> file(new UnmanagedMemoryFile(data, size));
    DVASSERT(file);

    Vector<Image*> images;
    ImageSystem::Load(file, images);
    if (images.size() == 0)
    {
        return nullptr;
    }

    Sprite* sprite = CreateFromImage(images[0], contentScaleIncluded, inVirtualSpace);

    for_each(images.begin(), images.end(), SafeRelease<Image>);

    return sprite;
}

String Sprite::GetPathString(const Sprite* sprite)
{
    if (nullptr == sprite)
        return String();

    FilePath path(sprite->GetRelativePathname());

    String pathName;
    if (!path.IsEmpty())
    {
        pathName = path.GetFrameworkPath();
    }
    return pathName;
}

Sprite* Sprite::CreateFromSourceFile(const FilePath& path, bool contentScaleIncluded /* = false*/, bool inVirtualSpace /* = false */)
{
    Sprite* sprite = GetSpriteFromMap(path);
    if (sprite != nullptr)
    {
        return sprite;
    }

    Vector<Image*> images;
    ImageSystem::Load(path, images);
    if (images.size() == 0)
    {
        return nullptr;
    }

    sprite = CreateFromImage(images[0], contentScaleIncluded, inVirtualSpace);
    if (sprite)
    {
        sprite->SetRelativePathname(path);
    }

    for_each(images.begin(), images.end(), SafeRelease<Image>);

    return sprite;
}

void Sprite::InitFromTexture(Texture* fromTexture, int32 xOffset, int32 yOffset, float32 sprWidth, float32 sprHeight, int32 targetWidth, int32 targetHeight, bool contentScaleIncluded, const FilePath& spriteName /* = FilePath() */)
{
    Clear();

    Vector2 offset((float32(xOffset)), (float32(yOffset)));
    size = Vector2(sprWidth, sprHeight);
    if (!contentScaleIncluded)
    {
        offset = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysical(offset);
    }
    else
    {
        size = GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtual(size);
    }

    resourceSizeIndex = GetEngineContext()->uiControlSystem->vcs->GetBaseResourceIndex();

    type = SPRITE_FROM_TEXTURE;
    textureCount = 1;
    textures = new Texture*[textureCount];
    textureNames = new FilePath[textureCount];
    textureInVirtualSpace = contentScaleIncluded;

    textures[0] = SafeRetain(fromTexture);
    if (textures[0])
    {
        textureNames[0] = textures[0]->GetPathname();
    }

    defaultPivotPoint.x = 0;
    defaultPivotPoint.y = 0;
    frameCount = 1;

    texCoords = new float32*[frameCount];
    frameVertices = new float32*[frameCount];
    rectsAndOffsets = new float32*[frameCount];
    frameTextureIndex = new int32[frameCount];

    for (int i = 0; i < frameCount; i++)
    {
        frameVertices[i] = new float32[8];
        texCoords[i] = new float32[8];
        rectsAndOffsets[i] = new float32[6];
        frameTextureIndex[i] = 0;

        float32 x, y, dx, dy, xOff, yOff;
        x = offset.x;
        y = offset.y;
        dx = (targetWidth == -1) ? GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(size.x) : float32(targetWidth);
        dy = (targetHeight == -1) ? GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(size.y) : float32(targetHeight);
        xOff = 0;
        yOff = 0;

        float32* rectAndOffset = rectsAndOffsets[i];
        rectAndOffset[eRectsAndOffsets::X_POSITION_IN_TEXTURE] = x;
        rectAndOffset[eRectsAndOffsets::Y_POSITION_IN_TEXTURE] = y;
        rectAndOffset[eRectsAndOffsets::ACTIVE_WIDTH] = size.x;
        rectAndOffset[eRectsAndOffsets::ACTIVE_HEIGHT] = size.y;
        rectAndOffset[eRectsAndOffsets::X_OFFSET_TO_ACTIVE] = xOff;
        rectAndOffset[eRectsAndOffsets::Y_OFFSET_TO_ACTIVE] = yOff;

        float32* frameVerts = frameVertices[i];
        frameVerts[0] = xOff;
        frameVerts[1] = yOff;
        frameVerts[2] = xOff + size.x;
        frameVerts[3] = yOff;
        frameVerts[4] = xOff;
        frameVerts[5] = (yOff + size.y);
        frameVerts[6] = (xOff + size.x);
        frameVerts[7] = (yOff + size.y);

        dx += x;
        dy += y;

        int32 frameIndex = frameTextureIndex[i];
        Texture* texture = textures[frameIndex];
        float32* texCoord = texCoords[i];

        float32 textureX = x / texture->width;
        float32 textureDX = dx / texture->width;
        float32 textureY = y / texture->height;
        float32 textureDY = dy / texture->height;

        texCoord[0] = textureX;
        texCoord[1] = textureY;
        texCoord[2] = textureDX;
        texCoord[3] = textureY;
        texCoord[4] = textureX;
        texCoord[5] = textureDY;
        texCoord[6] = textureDX;
        texCoord[7] = textureDY;
    }

    // DF-1984 - Set available sprite relative path name here. Use FBO sprite name only if sprite name is empty.
    if (relativePathname.IsEmpty())
    {
        relativePathname = spriteName.IsEmpty() ? Format("FBO sprite %d", fboCounter) : spriteName;
    }

    spriteMapMutex.Lock();
    spriteMap[FILEPATH_MAP_KEY(relativePathname)] = this;
    spriteMapMutex.Unlock();

    fboCounter++;
    Reset();
}

void Sprite::Clear()
{
    for (int32 k = 0; k < textureCount; ++k)
    {
        SafeRelease(textures[k]);
    }
    SafeDeleteArray(textures);
    SafeDeleteArray(textureNames);

    if (frameVertices != 0)
    {
        for (int i = 0; i < frameCount; i++)
        {
            SafeDeleteArray(frameVertices[i]);
            SafeDeleteArray(texCoords[i]);
            SafeDeleteArray(rectsAndOffsets[i]);
        }
    }

    SafeDeleteArray(frameVertices);
    SafeDeleteArray(texCoords);
    SafeDeleteArray(rectsAndOffsets);
    SafeDeleteArray(frameTextureIndex);
    textureCount = 0;
    rectsAndOffsetsOriginal.clear();
}

Sprite::~Sprite()
{
    spriteMapMutex.Lock();
    spriteMap.erase(FILEPATH_MAP_KEY(relativePathname));
    spriteMapMutex.Unlock();
    GetEngineContext()->dynamicAtlasSystem->UnregisterSprite(this);
    Clear();
}

Texture* Sprite::GetTexture() const
{
    return textures[0];
}

Texture* Sprite::GetTexture(int32 frameNumber) const
{
    frameNumber = Clamp(frameNumber, 0, frameCount - 1);
    return textures[frameTextureIndex[frameNumber]];
}

float32* Sprite::GetTextureVerts(int32 frameNumber)
{
    frameNumber = Clamp(frameNumber, 0, frameCount - 1);
    return texCoords[frameNumber];
}

int32 Sprite::GetFrameCount() const
{
    return frameCount;
}

float32 Sprite::GetWidth() const
{
    return size.dx;
}

float32 Sprite::GetHeight() const
{
    return size.dy;
}

const Vector2& Sprite::GetSize() const
{
    return size;
}

const Vector2& Sprite::GetDefaultPivotPoint() const
{
    return defaultPivotPoint;
}

void Sprite::SetDefaultPivotPoint(float32 x, float32 y)
{
    defaultPivotPoint.x = x;
    defaultPivotPoint.y = y;
}

void Sprite::SetDefaultPivotPoint(const Vector2& newPivotPoint)
{
    defaultPivotPoint = newPivotPoint;
}

int32 Sprite::GetFrameByName(const FastName& frameName) const
{
    if (!frameName.IsValid())
    {
        return INVALID_FRAME_INDEX;
    }

    for (int32 i = 0; i < frameCount; i++)
    {
        if (frameNames[i] == frameName)
            return i;
    }

    return INVALID_FRAME_INDEX;
}

void Sprite::Reset()
{
    flags = 0;
    modification = 0;
    clipPolygon = 0;
}

float32 Sprite::GetRectOffsetValueForFrame(int32 frame, eRectsAndOffsets valueType) const
{
    int32 clampedFrame = Clamp(frame, 0, frameCount - 1);
    return rectsAndOffsets[clampedFrame][valueType];
}

const float32* Sprite::GetFrameVerticesForFrame(int32 frame) const
{
    int32 clampedFrame = Clamp(frame, 0, frameCount - 1);
    return frameVertices[clampedFrame];
}

const float32* Sprite::GetTextureCoordsForFrame(int32 frame) const
{
    int32 clampedFrame = Clamp(frame, 0, frameCount - 1);
    return texCoords[clampedFrame];
}

void Sprite::PrepareForNewSize()
{
    if (relativePathname.IsEmpty())
        return;

    // Check if sprite exists by trying to open it.
    // If file doesn't exists preparation can't be continued
    {
        int resIndex = 0;

        File* fp = GetSpriteFile(relativePathname, resIndex);
        if (fp == nullptr)
            return;

        SafeRelease(fp);
    }

    Vector2 tempPivotPoint = defaultPivotPoint;

    Clear();

    spriteMapMutex.Lock();
    spriteMap.erase(FILEPATH_MAP_KEY(relativePathname));
    spriteMapMutex.Unlock();
    GetEngineContext()->dynamicAtlasSystem->UnregisterSprite(this);

    textures = 0;
    textureNames = 0;

    frameTextureIndex = 0;
    textureCount = 0;

    frameVertices = 0;
    texCoords = 0;
    rectsAndOffsets = 0;

    size.dx = 24;
    size.dy = 24;
    frameCount = 0;

    modification = 0;
    flags = 0;
    resourceSizeIndex = 0;

    clipPolygon = 0;

    PureCreate(relativePathname, this);
    //TODO: следующая строка кода написада здесь только до тех времен
    //		пока defaultPivotPoint не начнет задаваться прямо в спрайте,
    //		но возможно это навсегда.
    defaultPivotPoint = tempPivotPoint;
}

void Sprite::ValidateForSize()
{
    Logger::FrameworkDebug("--------------- Sprites validation for new resolution ----------------");
    List<Sprite*> spritesToReload;

    spriteMapMutex.Lock();
    for (SpriteMap::iterator it = spriteMap.begin(); it != spriteMap.end(); ++it)
    {
        Sprite* sp = it->second;
        if (sp->type == SPRITE_FROM_FILE && GetEngineContext()->uiControlSystem->vcs->GetDesirableResourceIndex() != sp->GetResourceSizeIndex())
        {
            spritesToReload.push_back(sp);
        }
    }
    spriteMapMutex.Unlock();

    for (List<Sprite*>::iterator it = spritesToReload.begin(); it != spritesToReload.end(); ++it)
    {
        (*it)->PrepareForNewSize();
    }
    Logger::FrameworkDebug("----------- Sprites validation for new resolution DONE  --------------");
    // Texture::DumpTextures();
}

void Sprite::DumpSprites()
{
    Logger::FrameworkDebug("============================================================");
    Logger::FrameworkDebug("--------------- Currently allocated sprites ----------------");

    spriteMapMutex.Lock();
    uint32 spritesCount = static_cast<uint32>(spriteMap.size());
    for (SpriteMap::iterator it = spriteMap.begin(); it != spriteMap.end(); ++it)
    {
        Sprite* sp = it->second; //[spriteDict objectForKey:[txKeys objectAtIndex:i]];
        Logger::FrameworkDebug("name:%s count:%d size(%.0f x %.0f)", sp->relativePathname.GetAbsolutePathname().c_str(), sp->GetRetainCount(), sp->size.dx, sp->size.dy);
    }
    spriteMapMutex.Unlock();

    Logger::FrameworkDebug("Total spritesCount: %d", spritesCount);
    Logger::FrameworkDebug("============================================================");
}

void Sprite::SetClipPolygon(Polygon2* _clipPolygon)
{
    clipPolygon = _clipPolygon;
}

void Sprite::ConvertToVirtualSize()
{
    frameVertices[0][0] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(frameVertices[0][0], resourceSizeIndex);
    frameVertices[0][1] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(frameVertices[0][1], resourceSizeIndex);
    frameVertices[0][2] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(frameVertices[0][2], resourceSizeIndex);
    frameVertices[0][3] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(frameVertices[0][3], resourceSizeIndex);
    frameVertices[0][4] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(frameVertices[0][4], resourceSizeIndex);
    frameVertices[0][5] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(frameVertices[0][5], resourceSizeIndex);
    frameVertices[0][6] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(frameVertices[0][6], resourceSizeIndex);
    frameVertices[0][7] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(frameVertices[0][7], resourceSizeIndex);

    frameVertices[0][0] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(frameVertices[0][0]);
    frameVertices[0][1] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(frameVertices[0][1]);
    frameVertices[0][2] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(frameVertices[0][2]);
    frameVertices[0][3] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(frameVertices[0][3]);
    frameVertices[0][4] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(frameVertices[0][4]);
    frameVertices[0][5] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(frameVertices[0][5]);
    frameVertices[0][6] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalX(frameVertices[0][6]);
    frameVertices[0][7] = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToPhysicalY(frameVertices[0][7]);

    texCoords[0][0] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(texCoords[0][0], resourceSizeIndex);
    texCoords[0][1] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(texCoords[0][1], resourceSizeIndex);
    texCoords[0][2] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(texCoords[0][2], resourceSizeIndex);
    texCoords[0][3] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(texCoords[0][3], resourceSizeIndex);
    texCoords[0][4] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(texCoords[0][4], resourceSizeIndex);
    texCoords[0][5] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(texCoords[0][5], resourceSizeIndex);
    texCoords[0][6] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualX(texCoords[0][6], resourceSizeIndex);
    texCoords[0][7] = GetEngineContext()->uiControlSystem->vcs->ConvertResourceToVirtualY(texCoords[0][7], resourceSizeIndex);
}

const FilePath& Sprite::GetRelativePathname() const
{
    return relativePathname;
}

void SpriteDrawState::BuildStateFromParentAndLocal(const SpriteDrawState& parentState, const SpriteDrawState& localState)
{
    position.x = parentState.position.x + localState.position.x * parentState.scale.x;
    position.y = parentState.position.y + localState.position.y * parentState.scale.y;
    if (parentState.angle != 0)
    {
        float tmpX = position.x;
        position.x = (tmpX - parentState.position.x) * parentState.cosA + (parentState.position.y - position.y) * parentState.sinA + parentState.position.x;
        position.y = (tmpX - parentState.position.x) * parentState.sinA + (position.y - parentState.position.y) * parentState.cosA + parentState.position.y;
    }
    scale.x = localState.scale.x * parentState.scale.x;
    scale.y = localState.scale.y * parentState.scale.y;
    angle = localState.angle + parentState.angle;
    if (angle != precomputedAngle) // compute precomputed angle and store values
    {
        precomputedAngle = angle;
        if (precomputedAngle != parentState.angle)
        {
            cosA = std::cos(precomputedAngle);
            sinA = std::sin(precomputedAngle);
        }
        else
        {
            cosA = parentState.cosA;
            sinA = parentState.sinA;
        }
    }
    pivotPoint.x = localState.pivotPoint.x;
    pivotPoint.y = localState.pivotPoint.y;

    frame = localState.frame;
}

void Sprite::ReloadSprites()
{
    ReloadSprites(Texture::GetPrimaryGPUForLoading());
}

void Sprite::ReloadSprites(eGPUFamily gpu)
{
    for (SpriteMap::iterator it = spriteMap.begin(); it != spriteMap.end(); ++it)
    {
        (it->second)->Reload(gpu);
    }
}

void Sprite::Reload()
{
    Reload(Texture::GetPrimaryGPUForLoading());
}

void Sprite::Reload(eGPUFamily gpu)
{
    if (type == SPRITE_FROM_FILE)
    {
        GetEngineContext()->dynamicAtlasSystem->UnregisterSprite(this);
        ReloadExistingTextures(gpu);
        Clear();

        File* fp = GetSpriteFile(relativePathname, resourceSizeIndex);
        if (fp)
        {
            InitFromFile(fp);
            SafeRelease(fp);
        }
        else
        {
            Logger::Warning("Unable to reload sprite %s", relativePathname.GetAbsolutePathname().c_str());

            Texture* pinkTexture = Texture::CreatePink();
            InitFromTexture(pinkTexture, 0, 0, 16.0f, 16.0f, 16, 16, false, relativePathname);
            pinkTexture->Release();

            type = SPRITE_FROM_FILE;
        }
    }
}

File* Sprite::GetSpriteFile(const FilePath& spriteName, int32& resourceSizeIndex)
{
    FilePath pathName = FilePath::CreateWithNewExtension(spriteName, ".txt");
    FilePath scaledPath = GetScaledName(pathName);

    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;

    FilePath texturePath;
    File* fp = LoadLocalizedFile(scaledPath, texturePath);
    if (!fp)
    {
        if (vcs->GetResourceFoldersCount() > 1)
        { // try to load default path in case of several resource folders
            fp = LoadLocalizedFile(pathName, texturePath);
        }

        if (!fp)
        {
            Logger::Warning("Failed to open sprite file: %s", pathName.GetAbsolutePathname().c_str());
            return NULL;
        }

        resourceSizeIndex = vcs->GetBaseResourceIndex();
    }
    else
    {
        resourceSizeIndex = vcs->GetDesirableResourceIndex();
    }

    return fp;
}

void Sprite::ReloadExistingTextures(eGPUFamily gpu)
{
    //this function need to be sure that textures really would reload
    for (int32 i = 0; i < textureCount; ++i)
    {
        if (textures[i])
        {
            if (!textures[i]->GetPathname().IsEmpty())
            {
                if (FileSystem::Instance()->Exists(textures[i]->GetPathname()))
                {
                    textures[i]->ReloadAs(gpu);
                }
            }
            else if (!textures[i]->IsPinkPlaceholder())
            {
                Logger::Error("[Sprite::ReloadSpriteTextures] Something strange with texture_%d", i);
            }
        }
    }
}

void Sprite::SetRelativePathname(const FilePath& path)
{
    spriteMapMutex.Lock();
    spriteMap.erase(FILEPATH_MAP_KEY(relativePathname));
    relativePathname = path;
    spriteMap[FILEPATH_MAP_KEY(this->relativePathname)] = this;
    spriteMapMutex.Unlock();
    GetTexture()->SetPathname(path);
}
};
