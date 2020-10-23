#pragma once

#include "Classes/Collada/ColladaToSc2Importer/ImportLibrary.h"
#include "Classes/Collada/ColladaErrorCodes.h"
#include "Classes/Collada/ColladaMeshInstance.h"

#include <Base/FastName.h>
#include <Render/Highlevel/RenderObject.h>
#include <Scene3D/Entity.h>

namespace DAVA
{
class Entity;
class ColladaSceneNode;

class ColladaImporter
{
public:
    ColladaImporter();
    eColladaErrorCodes SaveSC2(ColladaScene* colladaScene, const FilePath& scenePath);
    eColladaErrorCodes SaveAnimations(ColladaScene* colladaScene, const FilePath& dir);

private:
    void ImportAnimation(ColladaSceneNode* colladaNode, Entity* nodeEntity);
    void ImportSkeleton(ColladaSceneNode* colladaNode, Entity* node);
    void LoadMaterialParents(ColladaScene* colladaScene);
    void LoadAnimations(ColladaScene* colladaScene);
    bool VerifyColladaMesh(ColladaMeshInstance* mesh, const FastName& nodeName);
    eColladaErrorCodes VerifyDavaMesh(RenderObject* mesh, const FastName name);
    eColladaErrorCodes ImportMeshes(const Vector<ColladaMeshInstance*>& meshInstances, Entity* node);
    eColladaErrorCodes BuildSceneAsCollada(Entity* root, ColladaSceneNode* colladaNode);
    RenderObject* GetMeshFromCollada(ColladaMeshInstance* mesh, const FastName& name);

    ImportLibrary library;
};
};