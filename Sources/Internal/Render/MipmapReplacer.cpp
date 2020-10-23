#include "Render/MipmapReplacer.h"
#include "Render/Texture.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Image/Image.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/PixelFormatDescriptor.h"
#include "UI/UIScreenManager.h"

namespace DAVA
{
const static int32 DEFAULT_INTERNAL_DATA_SIZE = 2048 * 2048;

uint32* MipMapReplacer::mipmapData = nullptr;
int32 MipMapReplacer::mipmapDataSize = 0;

void MipMapReplacer::AllocateInternalDataIfNeeded(int32 requestedSize)
{
    if (mipmapDataSize < requestedSize)
    {
        SafeDeleteArray(mipmapData);
        mipmapDataSize = requestedSize;
        mipmapData = new uint32[mipmapDataSize];
    }
}

void MipMapReplacer::ReleaseInternalData()
{
    SafeDeleteArray(mipmapData);
    mipmapDataSize = 0;
}

void MipMapReplacer::ReplaceMipMaps(Entity* node, const FastName& textureName /* = NMaterial::TEXTURE_ALBEDO */)
{
    Set<Texture*> textures;
    EnumerateTexturesRecursive(node, textures, textureName);

    if (!textures.size())
        return;

    AllocateInternalDataIfNeeded(DEFAULT_INTERNAL_DATA_SIZE);

    Set<Texture*>::iterator endIt = textures.end();
    for (Set<Texture*>::iterator it = textures.begin(); it != endIt; ++it)
        ReplaceMipMaps((*it));

    ReleaseInternalData();
}

void MipMapReplacer::EnumerateTexturesRecursive(Entity* entity, Set<Texture*>& textures, const FastName& textureName)
{
    if (!entity)
        return;
#if RHI_COMPLETE
    int32 childrenCount = entity->GetChildrenCount();
    for (int32 i = 0; i < childrenCount; i++)
        EnumerateTexturesRecursive(entity->GetChild(i), textures, textureName);

    RenderObject* ro = GetRenderObject(entity);
    if (ro)
    {
        int32 rbCount = ro->GetRenderBatchCount();
        for (int32 i = 0; i < rbCount; i++)
        {
            RenderBatch* rb = ro->GetRenderBatch(i);
            if (rb)
            {
                NMaterial* material = rb->GetMaterial();
                while (material)
                {
                    Texture* texture = material->GetTexture(textureName);
                    if (texture)
                        textures.insert(texture);

                    material = material->GetParent();
                }
            }
        }
    }
#endif
}

void MipMapReplacer::ReplaceMipMaps(Texture* texture)
{
#if RHI_COMPLETE
    if (!texture)
        return;

    static uint32 mipmapColor[8] = {
        0xff00ff00, //green
        0xff0000ff, //red
        0xffff0000, //blue
        0xff00ffff, //yellow
        0xffff00ff, //pink
        0xffffff00, //cyan
        0xffffffff, //white
        0xff000000, //black
    };

    static GLuint CUBE_FACE_GL_NAMES[] =
    {
      GL_TEXTURE_CUBE_MAP_POSITIVE_X,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    int32 width = texture->GetWidth();
    int32 height = texture->GetHeight();

    AllocateInternalDataIfNeeded(width * height);

    int32 saveId = RenderManager::Instance()->HWglGetLastTextureID(texture->textureType);
    RenderManager::Instance()->HWglBindTexture(texture->id, texture->textureType);
    RENDER_VERIFY(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    int32 miplevel = 0;
    bool isCubemap = texture->GetDescriptor()->IsCubeMap();
    while (width > 0 || height > 0)
    {
        if (width == 0)
            width = 1;
        if (height == 0)
            height = 1;

        const uint32& color = mipmapColor[miplevel % 8];
        const uint32 dataCount = width * height;
        for (uint32 i = 0; i < dataCount; ++i)
            mipmapData[i] = color;

        if (!isCubemap)
        {
            RENDER_VERIFY(glTexImage2D(GL_TEXTURE_2D, miplevel, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmapData));
        }
        else
        {
            for (int32 i = 0; i < 6; ++i)
            {
                RENDER_VERIFY(glTexImage2D(CUBE_FACE_GL_NAMES[i], miplevel, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmapData));
            }
        }

        width /= 2;
        height /= 2;
        miplevel++;
    }

    if (0 != saveId)
    {
        RenderManager::Instance()->HWglBindTexture(saveId, texture->textureType);
    }
#endif //RHI_COMPLETE
}
};
