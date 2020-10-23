#include "ChangeLODDistanceCommand.h"
#include "Commands2/RECommandIDs.h"

using namespace DAVA;

ChangeLODDistanceCommand::ChangeLODDistanceCommand(DAVA::LodComponent* lod, DAVA::int32 lodLayer, DAVA::float32 distance)
    : RECommand(CMDID_LOD_DISTANCE_CHANGE, "Change LOD Distance")
    , lodComponent(lod)
    , layer(lodLayer)
    , newDistance(distance)
    , oldDistance(0)
{
}

void ChangeLODDistanceCommand::Redo()
{
    if (!lodComponent)
        return;

    oldDistance = lodComponent->GetLodLayerDistance(layer);
    lodComponent->SetLodLayerDistance(layer, newDistance);
}

void ChangeLODDistanceCommand::Undo()
{
    if (!lodComponent)
        return;

    lodComponent->SetLodLayerDistance(layer, oldDistance);
}

Entity* ChangeLODDistanceCommand::GetEntity() const
{
    if (lodComponent)
        return lodComponent->GetEntity();

    return nullptr;
}
