#include "Commands2/ParticleForceSimplifiedRemoveCommand.h"
#include "Commands2/RECommandIDs.h"

ParticleForceSimplifiedRemoveCommand::ParticleForceSimplifiedRemoveCommand(DAVA::ParticleForceSimplified* _force, DAVA::ParticleLayer* _layer)
    : RECommand(CMDID_PARTICLE_LAYER_REMOVE, "Remove simplified particle force")
    , force(_force)
    , layer(_layer)
{
    SafeRetain(force);
}

ParticleForceSimplifiedRemoveCommand::~ParticleForceSimplifiedRemoveCommand()
{
    SafeRelease(force);
}

void ParticleForceSimplifiedRemoveCommand::Undo()
{
    if (NULL != layer && NULL != force)
    {
        layer->AddSimplifiedForce(force);
    }
}

void ParticleForceSimplifiedRemoveCommand::Redo()
{
    if (NULL != layer && NULL != force)
    {
        layer->RemoveSimplifiedForce(force);
    }
}
