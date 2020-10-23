#include "Commands2/TransformCommand.h"
#include "Commands2/RECommandIDs.h"
#include "Scene3D/Entity.h"

TransformCommand::TransformCommand(Selectable object_, const DAVA::Matrix4& origTransform_, const DAVA::Matrix4& newTransform_)
    : RECommand(CMDID_TRANSFORM, "Transform")
    , object(object_)
    , undoTransform(origTransform_)
    , redoTransform(newTransform_)
{
}

void TransformCommand::Undo()
{
    object.SetLocalTransform(undoTransform);
}

void TransformCommand::Redo()
{
    object.SetLocalTransform(redoTransform);
}

const Selectable& TransformCommand::GetTransformedObject() const
{
    return object;
}

DAVA::Entity* TransformCommand::GetEntity() const
{
    return object.AsEntity();
}
