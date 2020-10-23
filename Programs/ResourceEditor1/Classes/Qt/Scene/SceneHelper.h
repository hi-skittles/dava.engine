#ifndef __SCENE_HELPER_H__
#define __SCENE_HELPER_H__

#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Render/Texture.h"
#include "FileSystem/FilePath.h"

namespace SceneHelper
{
class TextureCollector
{
public:
    enum Options
    {
        Default = 0,
        IncludeNullTextures = 0x1,
        OnlyActiveTextures = 0x2
    };

    TextureCollector(DAVA::uint32 options = Default);

    void Apply(DAVA::NMaterial* material);
    DAVA::TexturesMap& GetTextures();

private:
    bool includeNullTextures = true;
    bool onlyActiveTextures = false;
    DAVA::TexturesMap textureMap;
};

void EnumerateSceneTextures(DAVA::Scene* forScene, TextureCollector& collector);
void EnumerateEntityTextures(DAVA::Scene* forScene, DAVA::Entity* forNode, TextureCollector& collector);

// enumerates materials from render batches and their parents
void EnumerateMaterials(DAVA::Entity* forNode, DAVA::Set<DAVA::NMaterial*>& materials);

// enumerates only materials from render batches
void EnumerateMaterialInstances(DAVA::Entity* forNode, DAVA::Set<DAVA::NMaterial*>& materials);

DAVA::int32 EnumerateModifiedTextures(DAVA::Scene* forScene, DAVA::Map<DAVA::Texture*, DAVA::Vector<DAVA::eGPUFamily>>& textures);

DAVA::Entity* CloneEntityWithMaterials(DAVA::Entity* fromNode);

void BuildMaterialList(DAVA::Entity* forNode, DAVA::Set<DAVA::NMaterial*>& materialList, bool includeRuntime = true);

bool IsEntityChildRecursive(DAVA::Entity* root, DAVA::Entity* child);
}

#endif // __SCENE_HELPER_H__
