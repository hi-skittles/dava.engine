#pragma once

#include "Commands2/Base/RECommand.h"
#include "DAVAEngine.h"

class ChangeLODDistanceCommand : public RECommand
{
public:
    ChangeLODDistanceCommand(DAVA::LodComponent* lod, DAVA::int32 lodLayer, DAVA::float32 distance);

    void Undo() override;
    void Redo() override;
    DAVA::Entity* GetEntity() const;

protected:
    DAVA::LodComponent* lodComponent;
    DAVA::int32 layer;
    DAVA::float32 newDistance;
    DAVA::float32 oldDistance;
};
