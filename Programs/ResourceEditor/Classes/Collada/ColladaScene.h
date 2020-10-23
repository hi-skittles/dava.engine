#ifndef __COLLADALOADER_COLLADASCENE_H__
#define __COLLADALOADER_COLLADASCENE_H__

#include "ColladaIncludes.h"
#include "ColladaSceneNode.h"
#include "ColladaMesh.h"
#include "ColladaLight.h"
#include "ColladaMaterial.h"
#include "ColladaMeshInstance.h"
#include "ColladaSkinnedMesh.h"
#include "ColladaCamera.h"
#include "ColladaAnimation.h"

namespace DAVA
{
Matrix4 ConvertMatrix(FMMatrix44& matrix);

class ColladaScene
{
public:
    ColladaScene(FCDSceneNode* rootNode);
    ~ColladaScene();

    void ExportAnimations(ColladaAnimation* anim, FCDSceneNode* currentNode, float32 anStart, float32 anEnd);
    void ExportScene(FCDSceneNode* fcdNode = 0, ColladaSceneNode* node = 0);
    void Render();

    void RenderAxes();
    void RenderGrid();

    void SetupDefaultLights();

    ColladaMeshInstance* CreateMeshInstance(ColladaMesh* mesh, FCDGeometryInstance* geometryInstance, ColladaSkinnedMesh* skinned = nullptr);

    ColladaMesh* FindMeshWithName(const fm::string& name);
    ColladaTexture* FindTextureWithName(const fm::string& name);
    ColladaMaterial* FindMaterialWithName(const fm::string& name);
    ColladaLight* FindLightWithName(const fm::string& name);
    ColladaSkinnedMesh* FindSkinnedMeshWithName(const fm::string& name);

    int FindMaterialIndex(ColladaMaterial* material);
    int FindMeshIndex(ColladaMesh* mesh);
    bool FindPolyGroupIndex(ColladaPolygonGroup* group, int& meshIndex, int& polygroupIndex);

    ColladaCamera* FindCameraWithName(const fm::string& name);
    int FindCameraIndex(ColladaCamera* cam);

    std::vector<ColladaMesh*> colladaMeshes;
    std::vector<ColladaLight*> colladaLights;
    std::vector<ColladaMaterial*> colladaMaterials;
    std::vector<ColladaTexture*> colladaTextures;
    std::vector<ColladaSkinnedMesh*> colladaSkinnedMeshes;

    std::vector<ColladaLight*> colladaActiveSceneLights;
    std::vector<ColladaCamera*> colladaCameras;

    std::vector<ColladaAnimation*> colladaAnimations;

    void SetExclusiveAnimation(int32 index);

    float32 animationStartTime;
    float32 animationEndTime;

    int exportSceneLevel;
    ColladaSceneNode* rootNode = nullptr;
    FCDSceneNode* rootFCDNode = nullptr;
    float32 currentTime = 0.f;
};
};

#endif // __COLLADALOADER_COLLADASCENE_H__
