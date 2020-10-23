#include "Render/3D/StaticMesh.h"
#include "Scene3D/DataNode.h"
#include "Scene3D/Scene.h"
#include "Logger/Logger.h"
#include "Render/Shader.h"
#include "Scene3D/SceneFileV2.h"

namespace DAVA
{
StaticMesh::StaticMesh(Scene* _scene)
    : DataNode()
{
    //    if (scene)
    //    {
    //        DataNode * staticMeshes = scene->GetStaticMeshes();
    //        staticMeshes->AddNode(this);
    //    }
}

void StaticMesh::SetScene(Scene* _scene)
{
    DVASSERT(scene == 0);
    scene = _scene;
    //    if (scene)
    //    {
    //        DataNode * staticMeshes = scene->GetStaticMeshes();
    //        staticMeshes->AddNode(this);
    //    }
}

int32 StaticMesh::Release()
{
    int32 retainCount = BaseObject::Release();
    //    if (retainCount == 1)
    //    {
    //        DataNode * staticMeshes = scene->GetStaticMeshes();
    //        staticMeshes->RemoveNode(this);
    //    }
    return retainCount;
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::AddNode(DataNode* node)
{
    PolygonGroup* group = dynamic_cast<PolygonGroup*>(node);
    DVASSERT(group != 0);
    if (group)
    {
        group->Retain();
        children.push_back(node);
    }
}

uint32 StaticMesh::GetPolygonGroupCount()
{
    return uint32(children.size());
}

PolygonGroup* StaticMesh::GetPolygonGroup(uint32 index)
{
    return reinterpret_cast<PolygonGroup*>(children[index]);
}
};