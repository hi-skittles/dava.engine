#include "Commands2/ParticleForceMoveCommand.h"
#include "Commands2/RECommandIDs.h"

#include <Particles/ParticleForce.h>
#include <Particles/ParticleForceSimplified.h>
#include <Particles/ParticleLayer.h>

ParticleSimplifiedForceMoveCommand::ParticleSimplifiedForceMoveCommand(DAVA::ParticleForceSimplified* _force, DAVA::ParticleLayer* _oldLayer, DAVA::ParticleLayer* _newLayer)
    : RECommand(CMDID_PARTICLE_SIMPLIFIED_FORCE_MOVE, "Move particle simplified force")
    , force(_force)
    , oldLayer(_oldLayer)
    , newLayer(_newLayer)
{
    SafeRetain(force);
}

ParticleSimplifiedForceMoveCommand::~ParticleSimplifiedForceMoveCommand()
{
    SafeRelease(force);
}

void ParticleSimplifiedForceMoveCommand::Undo()
{
    if (NULL != force)
    {
        if (NULL != newLayer)
        {
            newLayer->RemoveSimplifiedForce(force);
        }

        if (NULL != oldLayer)
        {
            oldLayer->AddSimplifiedForce(force);
        }
    }
}

void ParticleSimplifiedForceMoveCommand::Redo()
{
    if (NULL != force)
    {
        if (NULL != oldLayer)
        {
            oldLayer->RemoveSimplifiedForce(force);
        }

        if (NULL != newLayer)
        {
            newLayer->AddSimplifiedForce(force);
        }
    }
}

ParticleForceMoveCommand::ParticleForceMoveCommand(DAVA::ParticleForce* force, DAVA::ParticleLayer* oldLayer, DAVA::ParticleLayer* newLayer)
    : RECommand(CMDID_PARTICLE_FORCE_MOVE, "Move particle force")
    , force(force)
    , oldLayer(oldLayer)
    , newLayer(newLayer)
{
    SafeRetain(force);
}

ParticleForceMoveCommand::~ParticleForceMoveCommand()
{
    SafeRelease(force);
}

void ParticleForceMoveCommand::Undo()
{
    if (force != nullptr)
    {
        if (newLayer != nullptr)
        {
            newLayer->RemoveForce(force);
        }

        if (oldLayer != nullptr)
        {
            oldLayer->AddForce(force);
        }
    }
}

void ParticleForceMoveCommand::Redo()
{
    if (force != nullptr)
    {
        if (oldLayer != nullptr)
        {
            oldLayer->RemoveForce(force);
        }

        if (newLayer != nullptr)
        {
            newLayer->AddForce(force);
        }
    }
}
