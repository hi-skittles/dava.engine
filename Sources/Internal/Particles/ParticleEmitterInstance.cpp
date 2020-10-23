#include "Particles/ParticleEmitterInstance.h"

#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(ParticleEmitterInstance)
{
    ReflectionRegistrator<ParticleEmitterInstance>::Begin()
    .End();
}

ParticleEmitterInstance::ParticleEmitterInstance(ParticleEffectComponent* owner_)
    : owner(owner_)
{
}

ParticleEmitterInstance::ParticleEmitterInstance(ParticleEmitter* emitter)
    : emitter(SafeRetain(emitter))
{
}

ParticleEmitterInstance::ParticleEmitterInstance(ParticleEffectComponent* owner_, ParticleEmitter* emitter_)
    : owner(owner_)
    , emitter(SafeRetain(emitter_))
{
}

ParticleEmitterInstance* ParticleEmitterInstance::Clone() const
{
    ScopedPtr<ParticleEmitter> clonedEmitter(emitter->Clone());
    ParticleEmitterInstance* result = new ParticleEmitterInstance(owner, clonedEmitter.get());
    result->SetFilePath(GetFilePath());
    result->SetSpawnPosition(GetSpawnPosition());
    return result;
}
}
