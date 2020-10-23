#include "Scene3D/DataNode.h"
#include "FileSystem/KeyedArchive.h"
#include "Scene3D/SceneFileV2.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(DataNode)
{
    ReflectionRegistrator<DataNode>::Begin()
    .Field("id", &DataNode::id)[M::ReadOnly(), M::HiddenField()]
    .End();
}

DataNode::DataNode()
    : id(INVALID_ID)
    , isRuntime(false)
    , scene(nullptr)
{
}

DataNode::~DataNode()
{
}

void DataNode::SetScene(Scene* _scene)
{
    DVASSERT(scene == nullptr || scene == _scene);
    scene = _scene;
}

Scene* DataNode::GetScene() const
{
    return scene;
}

void DataNode::SetNodeID(uint64 _id)
{
    id = _id;
}

uint64 DataNode::GetNodeID() const
{
    return id;
}

bool DataNode::IsRuntime() const
{
    return isRuntime;
}

void DataNode::SetRuntime(bool _isRuntime)
{
    isRuntime = _isRuntime;
}

void DataNode::Load(KeyedArchive* archive, SerializationContext* serializationContext)
{
    BaseObject::LoadObject(archive);
    id = archive->GetByteArrayAsType<uint64>("#id", 0);
}

void DataNode::Save(KeyedArchive* archive, SerializationContext* serializationContext)
{
    BaseObject::SaveObject(archive);

    DVASSERT(INVALID_ID != id);
    archive->SetByteArrayAsType("#id", id);
}
}
