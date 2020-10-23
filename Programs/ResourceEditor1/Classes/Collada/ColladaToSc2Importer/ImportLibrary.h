#ifndef __DAVAENGINE_IMPORT_LIBRARY_H
#define __DAVAENGINE_IMPORT_LIBRARY_H

#include "Base/FastName.h"

namespace DAVA
{
class NMaterial;
class PolygonGroup;
class ColladaPolygonGroupInstance;
class AnimationData;
class ColladaMaterial;
class SceneNodeAnimation;

class ImportLibrary
{
public:
    ~ImportLibrary();

    PolygonGroup* GetOrCreatePolygon(ColladaPolygonGroupInstance* colladaPGI);
    NMaterial* CreateMaterialInstance(ColladaPolygonGroupInstance* colladaPolyGroupInst, const bool isShadow);
    NMaterial* GetOrCreateMaterialParent(ColladaMaterial* colladaMaterial, const bool isShadow);
    AnimationData* GetOrCreateAnimation(SceneNodeAnimation* colladaSceneNode);

private:
    Texture* GetTextureForPath(const FilePath& imagePath) const;
    void InitPolygon(PolygonGroup* davaPolygon, uint32 vertexFormat, Vector<ColladaVertex>& vertices) const;
    bool GetTextureTypeAndPathFromCollada(ColladaMaterial* material, FastName& type, FilePath& path) const;
    FilePath GetNormalMapTexturePath(const FilePath& originalTexturePath) const;
    inline void FlipTexCoords(Vector2& v) const;

    Map<ColladaPolygonGroupInstance*, PolygonGroup*> polygons;
    Map<FastName, NMaterial*> materialParents;
    Map<SceneNodeAnimation*, AnimationData*> animations;

    DAVA::uint32 materialInstanceNumber = 0;
};

inline void ImportLibrary::FlipTexCoords(Vector2& v) const
{
    v.y = 1.0f - v.y;
}
}

#endif //IMPORT_LIBRARY_H