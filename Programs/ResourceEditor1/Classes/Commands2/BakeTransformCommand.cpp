#include "Commands2/BakeTransformCommand.h"
#include "Commands2/RECommandIDs.h"
#include "Render/Highlevel/RenderObject.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Entity.h"

BakeGeometryCommand::BakeGeometryCommand(DAVA::Entity* entity_, DAVA::RenderObject* _object, DAVA::Matrix4 _transform)
    : RECommand(CMDID_BAKE_GEOMERTY, "Bake geometry")
    , entity(entity_)
    , object(_object)
    , transform(_transform)
{
}

BakeGeometryCommand::~BakeGeometryCommand()
{
}

void BakeGeometryCommand::Undo()
{
    if (NULL != object)
    {
        DAVA::Matrix4 undoTransform = transform;
        undoTransform.Inverse();

        object->BakeGeometry(undoTransform);
    }
}

void BakeGeometryCommand::Redo()
{
    if (NULL != object)
    {
        object->BakeGeometry(transform);
    }
}
