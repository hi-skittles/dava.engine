#include "Commands2/ParticleLayerMoveCommand.h"
#include "Commands2/RECommandIDs.h"

ParticleLayerMoveCommand::ParticleLayerMoveCommand(DAVA::ParticleEmitterInstance* oldEmitter_, DAVA::ParticleLayer* layer_,
                                                   DAVA::ParticleEmitterInstance* newEmitter_, DAVA::ParticleLayer* newBefore_ /* = nullptr */)
    : RECommand(CMDID_PARTICLE_LAYER_MOVE, "Move particle layer")
    , layer(layer_)
    , oldEmitter(oldEmitter_)
    , newEmitter(newEmitter_)
    , newBefore(newBefore_)
{
    SafeRetain(layer);

    if ((layer != nullptr) && (oldEmitter != nullptr))
    {
        oldBefore = oldEmitter->GetEmitter()->GetNextLayer(layer);
    }
}

ParticleLayerMoveCommand::~ParticleLayerMoveCommand()
{
    SafeRelease(layer);
}

void ParticleLayerMoveCommand::Undo()
{
    if (layer == nullptr)
        return;

    if (newEmitter != nullptr)
    {
        newEmitter->GetEmitter()->RemoveLayer(layer);
    }

    if (oldEmitter != nullptr)
    {
        if (oldBefore != nullptr)
        {
            oldEmitter->GetEmitter()->InsertLayer(layer, oldBefore);
        }
        else
        {
            oldEmitter->GetEmitter()->AddLayer(layer);
        }
    }
}

void ParticleLayerMoveCommand::Redo()
{
    if ((layer == nullptr) || (newEmitter == nullptr))
        return;

    if (nullptr != oldEmitter)
    {
        oldEmitter->GetEmitter()->RemoveLayer(layer);
    }

    if (nullptr != newBefore)
    {
        newEmitter->GetEmitter()->InsertLayer(layer, newBefore);
    }
    else
    {
        newEmitter->GetEmitter()->AddLayer(layer);
    }
}
