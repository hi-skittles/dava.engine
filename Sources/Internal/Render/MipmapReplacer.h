#ifndef __MIPMAP_REPLACER_H__
#define __MIPMAP_REPLACER_H__

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"
#include "Render/Material/NMaterial.h"
#include "Base/Observer.h"

namespace DAVA
{
class FilePath;
class Texture;
class Entity;
class MipMapReplacer
{
public:
    static void ReplaceMipMaps(Entity* entity, const FastName& textureName = NMaterialTextureName::TEXTURE_ALBEDO);

private:
    static void EnumerateTexturesRecursive(Entity* entity, Set<Texture*>& textures, const FastName& textureName);
    static void ReplaceMipMaps(Texture* texture);

    static void AllocateInternalDataIfNeeded(int32 requestedSize);
    static void ReleaseInternalData();

    static uint32* mipmapData;
    static int32 mipmapDataSize;
};
};

#endif // __MIPMAP_REPLACER_H__
