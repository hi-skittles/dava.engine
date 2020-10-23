#ifndef __PARTICLE_LAYER_REMOVE_COMMAND_H__
#define __PARTICLE_LAYER_REMOVE_COMMAND_H__

#include "Commands2/Base/RECommand.h"
#include "Particles/ParticleLayer.h"
#include "Particles/ParticleEmitter.h"

class ParticleLayerRemoveCommand : public RECommand
{
public:
    ParticleLayerRemoveCommand(DAVA::ParticleEmitter* emitter, DAVA::ParticleLayer* layer);
    ~ParticleLayerRemoveCommand();

    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* layer;
    DAVA::ParticleLayer* before;
    DAVA::ParticleEmitter* emitter;
};

#endif // __PARTICLE_LAYER_REMOVE_COMMAND_H__
