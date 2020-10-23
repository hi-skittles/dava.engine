#include "stdafx.h"
#include "ColladaSkinnedMesh.h"
#include "Scene3D/SceneNodeAnimation.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
ColladaSkinnedMesh::ColladaSkinnedMesh(FCDController* colladaController)
{
    controller = colladaController;
    bool isSkin = colladaController->IsSkin();

    if (isSkin)
    {
        FCDSkinController* skinController = colladaController->GetSkinController();
        size_t jointCount = skinController->GetJointCount();
        FCDSkinControllerJoint* origJoints = skinController->GetJoints();

        joints.resize(jointCount);
        for (size_t j = 0; j < jointCount; ++j)
        {
            joints[j].joint = &origJoints[j];
            joints[j].node = nullptr;
            joints[j].index = -1;
            joints[j].parentIndex = -1;
            joints[j].hierarhyDepth = 0;

            FMMatrix44 bindPoseInverse = joints[j].joint->GetBindPoseInverse();
            joints[j].colladaInverse0 = bindPoseInverse;
            joints[j].inverse0 = ConvertMatrix(joints[j].colladaInverse0);

            printf("%s, ", joints[j].joint->GetId().c_str());
        }

        colladaBindShapeMatrix = skinController->GetBindShapeTransform();
        bindShapeMatrix = ConvertMatrix(colladaBindShapeMatrix);

        printf("- controller: %s influence: %u influence-entity: %s\n", colladaController->GetDaeId().c_str(), static_cast<DAVA::uint32>(skinController->GetInfluenceCount()),
               skinController->GetTarget()->GetDaeId().c_str());

        vertexWeights.resize(skinController->GetInfluenceCount());
        int32 maxJoints = 0;
        for (int index = 0; index < (int)skinController->GetInfluenceCount(); ++index)
        {
            FCDSkinControllerVertex* vert = skinController->GetVertexInfluence(index);
            vertexWeights[index].jointCount = 0;
            for (int pp = 0; pp < (int)vert->GetPairCount(); ++pp)
            {
                FCDJointWeightPair* jointWeightPar = vert->GetPair(pp);

                vertexWeights[index].AddWeight(jointWeightPar->jointIndex, jointWeightPar->weight);
            }
            if (vertexWeights[index].jointCount > maxJoints)
                maxJoints = vertexWeights[index].jointCount;

            vertexWeights[index].Normalize();
        }
        printf("- max joints: %d\n", maxJoints);

        mesh = new ColladaMesh(colladaController->GetBaseGeometry()->GetMesh(), &(vertexWeights.front()), uint32(maxJoints));
    }
}

ColladaSkinnedMesh::~ColladaSkinnedMesh()
{
    SafeDelete(mesh);
}

void PrintMatrix(Matrix4& m, bool finishLine = true)
{
    for (int k = 0; k < 16; ++k)
        printf("%0.4f ", m.data[k]);
    if (finishLine)
        printf("\n");
}

void ColladaSkinnedMesh::UpdateSkinnedMesh(float32 time)
{
    if (mesh == 0)
        return;
    if (joints[0].node == 0)
        return;

    //	for (int k = 0; k < (int)joints.size(); ++k)
    //	{
    //		if (joints[k].node)
    //		{
    //			joints[k].colladaLocalTransform = joints[k].node->originalNode->CalculateLocalTransform();
    //			joints[k].colladaWorldTransform = joints[k].node->originalNode->CalculateWorldTransform();
    //		}
    //	}
    for (int k = 0; k < (int)joints.size(); ++k)
    {
        Joint& currentJoint = joints[k];
        if (joints[k].node)
        {
            //FMMatrix44 colladaLocalTransform = joints[k].node->originalNode->CalculateLocalTransform();
            //joints[k].localTransform = ConvertMatrix(colladaLocalTransform);
            joints[k].localTransform = joints[k].node->localTransform;
            // code to compute worldMatrixes using localMatrixes
            //			FMMatrix44 colladaWorldTransform = joints[k].node->originalNode->CalculateWorldTransform();
            //			Matrix4 worldTransform = ConvertMatrix(colladaWorldTransform);
            //			if (currentJoint.parentIndex != -1)
            //			{
            //				Joint & parentJoint = joints[currentJoint.parentIndex];
            //
            //				joints[k].worldTransform =  joints[k].localTransform * parentJoint.worldTransform;
            //			}else
            //			{
            //				FMMatrix44 realWorldTransform = joints[k].node->originalNode->CalculateWorldTransform();
            //				// get parent joint world transform
            //				joints[k].worldTransform = ConvertMatrix(realWorldTransform); ////joints[k].localTransform;
            //			}

            //printf("joint: %d: %s p:%d isEq: %d\n", k, currentJoint.node->originalNode->GetDaeId().c_str(), currentJoint.parentIndex, (int)(worldTransform == joints[k].worldTransform));
            //PrintMatrix(worldTransform);
            //PrintMatrix(joints[k].worldTransform);

            //			joints[k].localTranslation.x = joints[k].localTransform._30;
            //			joints[k].localTranslation.y = joints[k].localTransform._31;
            //			joints[k].localTranslation.z = joints[k].localTransform._32;
            //
            //			joints[k].localQuat.Construct(joints[k].localTransform);
            //
            //
            //			Matrix4 localTransformTrans;
            //			Matrix4 localTransformRot;
            //			Matrix4 localTransformFinal;
            //			localTransformTrans.CreateTranslation(joints[k].localTranslation);
            //			localTransformRot = joints[k].localQuat.GetMatrix();
            //
            //			joints[k].localTransform = localTransformRot * localTransformTrans;
            //
            //			FMMatrix44 colladaWorldTransform = joints[k].node->originalNode->CalculateWorldTransform();
            //			Matrix4 worldTransform = ConvertMatrix(colladaWorldTransform);
            if (currentJoint.parentIndex != -1)
            {
                Joint& parentJoint = joints[currentJoint.parentIndex];

                joints[k].worldTransform = joints[k].localTransform * parentJoint.worldTransform;
            }
            else
            {
                joints[k].worldTransform = joints[k].node->worldTransform; ////joints[k].localTransform;
            }
        }
    }

    for (int poly = 0; poly < (int)mesh->polygons.size(); ++poly)
    {
        ColladaPolygonGroup* polyGroup = mesh->polygons[poly];

        for (int vi = 0; vi < (int)polyGroup->skinVerteces.size(); ++vi)
        {
            ColladaVertex& v0 = polyGroup->unoptimizedVerteces[vi];
            ColladaVertex& v = polyGroup->skinVerteces[vi];

            v.position = Vector3(0.0, 0.0, 0.0);

            for (int jointIndex = 0; jointIndex < v.jointCount; ++jointIndex)
            {
                //FMVector3 pos(v0.position.x, v0.position.y, v0.position.z);
                //FMMatrix44 jointT = joints[v0.joint[jointIndex]].colladaWorldTransform * joints[v0.joint[jointIndex]].colladaInverse0 * bindShapeMatrix;
                //pos = jointT.TransformCoordinate(pos);

                Vector3 pos0(v0.position.x, v0.position.y, v0.position.z);
                Vector3 pos;
                Matrix4 finalJointMatrix = bindShapeMatrix * joints[v0.joint[jointIndex]].inverse0 * joints[v0.joint[jointIndex]].worldTransform;
                //				Matrix4 finalJointMatrix = joints[v0.joint[jointIndex]].worldTransform * joints[v0.joint[jointIndex]].inverse0 * bindShapeMatrix;
                pos = pos0 * finalJointMatrix;

                float weight = v0.weight[jointIndex];

                v.position.x += pos.x * weight;
                v.position.y += pos.y * weight;
                v.position.z += pos.z * weight;
            }
        }
    }
}

void ColladaSkinnedMesh::BuildJoints(ColladaSceneNode* node)
{
    sceneRootNode = node;
    LinkJoints(sceneRootNode);

    BuildJointsHierarhy();
}

void ColladaSkinnedMesh::LinkJoints(ColladaSceneNode* node, int32 parentJointIndex)
{
    int32 currentJointIndex = -1;
    for (int j = 0; j < (int)joints.size(); ++j)
    {
        if (joints[j].joint != nullptr && node->originalNode->GetSubId() == joints[j].joint->GetId())
        {
            Joint& joint = joints[j];

            joint.node = node;
            if (parentJointIndex != -1)
            {
                joint.index = j;
                joint.hierarhyDepth = joints[parentJointIndex].hierarhyDepth + 1;
            }

            joint.jointName = UTF8Utils::EncodeToUTF8(node->originalNode->GetName().c_str());
            joint.jointUID = node->originalNode->GetDaeId();

            node->inverse0 = joint.inverse0; // copy bindpos inverse matrix to node

            currentJointIndex = j;
            break;
        }
    }

    if (currentJointIndex == -1 && parentJointIndex != -1)
    {
        currentJointIndex = int32(joints.size());

        joints.emplace_back();
        Joint& joint = joints.back();

        joint.node = node;
        joint.joint = nullptr;
        joint.index = -1;
        joint.parentIndex = -1;

        Matrix4 bindPose;
        joints[parentJointIndex].inverse0.GetInverse(bindPose);
        bindPose = node->localTransform * bindPose;
        bindPose.GetInverse(joint.inverse0);

        joint.index = int32(joints.size() - 1);
        joint.hierarhyDepth = joints[parentJointIndex].hierarhyDepth + 1;

        joint.jointName = UTF8Utils::EncodeToUTF8(node->originalNode->GetName().c_str());
        joint.jointUID = node->originalNode->GetDaeId();

        node->inverse0 = joint.inverse0; // copy bindpos inverse matrix to node
    }

    for (int i = 0; i < (int)node->childs.size(); i++)
    {
        ColladaSceneNode* childNode = node->childs[i];
        LinkJoints(childNode, currentJointIndex);
    }
}

void ColladaSkinnedMesh::BuildJointsHierarhy()
{
    std::sort(joints.begin(), joints.end(), [](const Joint& l, const Joint& r)
              {
                  if (l.hierarhyDepth == r.hierarhyDepth)
                      return l.jointUID < r.jointUID;
                  else
                      return l.hierarhyDepth < r.hierarhyDepth;
              });

    //////////////////////////////////////////////////////////////////////////
    //rebind joints indices in geometry after sorting
    Map<int32, int32> sortedJointsMapping;
    for (int32 j = 0; j < int32(joints.size()); ++j)
        sortedJointsMapping[joints[j].index] = j;

    for (int32 p = 0; p < mesh->GetPolygonGroupCount(); ++p)
    {
        ColladaPolygonGroup* pg = mesh->GetPolygonGroup(p);

        std::vector<ColladaVertex>& vertices = pg->GetVertices();
        std::vector<ColladaVertex> originalVertices = vertices; //copy

        int32 vertexCount = int32(vertices.size());
        for (int32 v = 0; v < vertexCount; ++v)
        {
            for (int32 j = 0; j < vertices[v].jointCount; ++j)
            {
                vertices[v].joint[j] = sortedJointsMapping[originalVertices[v].joint[j]];
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    for (int32 j = 0; j < int32(joints.size()); ++j)
    {
        Joint& joint = joints[j];

        size_t parentIndex = std::distance(joints.begin(), std::find_if(joints.begin(), joints.end(), [&joint](const Joint& item) {
                                               return (item.node == joint.node->parent);
                                           }));

        joint.parentIndex = (parentIndex == joints.size()) ? -1 : int32(parentIndex);
        joint.index = j;
    }
}
};
