#ifndef __COLLADALOADER_COLLADAANIMATEDMESH_H__
#define __COLLADALOADER_COLLADAANIMATEDMESH_H__

#include "ColladaIncludes.h"
#include "ColladaSceneNode.h"
#include "ColladaMesh.h"

namespace DAVA
{
class ColladaSkinnedMesh
{
public:
    struct Joint
    {
        FCDSkinControllerJoint* joint;
        ColladaSceneNode* node;
        int32 index;
        int32 parentIndex;
        int32 hierarhyDepth;

        String jointName;
        String jointUID;

        // original collada matrices
        FMMatrix44 colladaInverse0;
        FMMatrix44 colladaLocalTransform;
        FMMatrix44 colladaWorldTransform;

        Matrix4 inverse0;
        Matrix4 localTransform;
        Matrix4 worldTransform;

        Quaternion localQuat;
        Vector3 localTranslation;
    };

    ColladaSkinnedMesh(FCDController* colladaController);
    ~ColladaSkinnedMesh();

    void BuildJoints(ColladaSceneNode* node);
    void UpdateSkinnedMesh(float32 time);

    std::vector<Joint> joints;
    std::vector<ColladaVertexWeight> vertexWeights;
    Matrix4 bindShapeMatrix;
    FMMatrix44 colladaBindShapeMatrix;

    ColladaSceneNode* sceneRootNode = nullptr;
    FCDController* controller = nullptr;

    ColladaMesh* mesh = nullptr;

private:
    void LinkJoints(ColladaSceneNode* node, int32 parentJointIndex = -1);
    void BuildJointsHierarhy();
};
};

#endif // __COLLADALOADER_COLLADACAMERA_H__
