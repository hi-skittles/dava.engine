#include "Commands2/ParticleEmitterMoveCommands.h"
#include "Commands2/RECommandIDs.h"

#include "Base/RefPtr.h"

ParticleEmitterMoveCommand::ParticleEmitterMoveCommand(DAVA::ParticleEffectComponent* oldEffect_, DAVA::ParticleEmitterInstance* emitter_,
                                                       DAVA::ParticleEffectComponent* newEffect_, int newIndex_)
    : RECommand(CMDID_PARTICLE_EMITTER_MOVE, "Move particle emitter")
    , oldEffect(oldEffect_)
    , newEffect(newEffect_)
    , oldIndex(-1)
    , newIndex(newIndex_)
{
    if (nullptr != emitter_ && nullptr != oldEffect_)
    {
        oldIndex = oldEffect->GetEmitterInstanceIndex(emitter_);
        instance = DAVA::RefPtr<DAVA::ParticleEmitterInstance>::ConstructWithRetain(oldEffect->GetEmitterInstance(oldIndex));
        DVASSERT(instance->GetEmitter() == emitter_->GetEmitter());
    }
}

void ParticleEmitterMoveCommand::Undo()
{
    if ((instance.Get() == nullptr) || (instance->GetEmitter() == nullptr))
        return;

    if (nullptr != newEffect)
    {
        newEffect->RemoveEmitterInstance(instance.Get());
    }

    if (nullptr != oldEffect)
    {
        if (-1 != oldIndex)
        {
            oldEffect->InsertEmitterInstanceAt(instance.Get(), oldIndex);
        }
        else
        {
            oldEffect->AddEmitterInstance(instance.Get());
        }
    }
}

void ParticleEmitterMoveCommand::Redo()
{
    if ((instance.Get() == nullptr) || (instance->GetEmitter() == nullptr) || (newEffect == nullptr))
        return;

    if (nullptr != oldEffect)
    {
        oldEffect->RemoveEmitterInstance(instance.Get());
    }

    if (-1 != newIndex)
    {
        newEffect->InsertEmitterInstanceAt(instance.Get(), newIndex);
    }
    else
    {
        newEffect->AddEmitterInstance(instance.Get());
    }
}
