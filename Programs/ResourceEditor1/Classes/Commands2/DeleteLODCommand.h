#ifndef __DELETE_LOD_COMMAND_H__
#define __DELETE_LOD_COMMAND_H__

#include "Commands2/Base/RECommand.h"

#include "Render/Highlevel/RenderBatch.h"
#include "Scene3D/Lod/LodComponent.h"

class DeleteRenderBatchCommand;
class DeleteLODCommand : public RECommand
{
public:
    DeleteLODCommand(DAVA::LodComponent* lod, DAVA::int32 lodIndex, DAVA::int32 switchIndex);
    virtual ~DeleteLODCommand();

    void Undo() override;
    void Redo() override;
    DAVA::Entity* GetEntity() const;

    const DAVA::Vector<DeleteRenderBatchCommand*>& GetRenderBatchCommands() const;

protected:
    DAVA::LodComponent* lodComponent;
    DAVA::int32 deletedLodIndex;
    DAVA::int32 requestedSwitchIndex;

    DAVA::Vector<DeleteRenderBatchCommand*> deletedBatches;
};


#endif // __DELETE_LOD_COMMAND_H__
