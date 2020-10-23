#pragma once

#include "Commands2/Base/RECommand.h"
#include "Particles/ParticleLayer.h"
#include "Particles/ParticleForceSimplified.h"

class ParticleForceSimplifiedRemoveCommand : public RECommand
{
public:
    ParticleForceSimplifiedRemoveCommand(DAVA::ParticleForceSimplified* force, DAVA::ParticleLayer* layer);
    ~ParticleForceSimplifiedRemoveCommand();

    void Undo() override;
    void Redo() override;

    DAVA::ParticleForceSimplified* force;
    DAVA::ParticleLayer* layer;
};