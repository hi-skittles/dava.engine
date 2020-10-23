#ifndef __DAVAENGINE_MESH_UTILS_H__
#define __DAVAENGINE_MESH_UTILS_H__

#include "PolygonGroup.h"
#include "Scene3D/Components/SkeletonComponent.h"
#include "Render/Highlevel/SkinnedMesh.h"

namespace DAVA
{
class RenderBatch;
class RenderObject;
class Entity;
namespace MeshUtils
{
void RebuildMeshTangentSpace(PolygonGroup* group, bool precomputeBinormal = true);
void CopyVertex(PolygonGroup* srcGroup, uint32 srcPos, PolygonGroup* dstGroup, uint32 dstPos);
void CopyGroupData(PolygonGroup* srcGroup, PolygonGroup* dstGroup);

/**
    Split geometry by max available joints per draw-call.
    Returns pairs [PolygonGroup, JointTargets].
*/
Vector<std::pair<PolygonGroup*, SkinnedMesh::JointTargets>> SplitSkinnedMeshGeometry(PolygonGroup* dataSource, uint32 maxJointCount);

/**
    Bake all geometry in entity hierarchy to one SkinnedMesh. Use entities as joints.
*/
SkinnedMesh* CreateHardSkinnedMesh(Entity* fromEntity, Vector<SkeletonComponent::Joint>& outJoints);

PolygonGroup* CreateShadowPolygonGroup(PolygonGroup* source);

Vector<uint16> BuildSortedIndexBufferData(PolygonGroup* pg, Vector3 direction);

uint32 ReleaseGeometryDataRecursive(Entity* forEntity);
};
};

#endif
