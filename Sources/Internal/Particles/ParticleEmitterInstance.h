#pragma once

#include "Particles/ParticleEmitter.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class ParticleEffectComponent;
class ParticleEmitterInstance : public BaseObject
{
public:
    ParticleEmitterInstance(ParticleEffectComponent* owner);
    ParticleEmitterInstance(ParticleEmitter* emitter);
    ParticleEmitterInstance(ParticleEffectComponent* owner, ParticleEmitter* emitter);

    ParticleEmitter* GetEmitter() const;
    const FilePath& GetFilePath() const;
    const Vector3& GetSpawnPosition() const;

    void SetEmitter(ParticleEmitter* emitter);
    void SetFilePath(const FilePath& filePath);
    void SetSpawnPosition(const Vector3& position);
    void SetOwner(ParticleEffectComponent* owner);

    ParticleEmitterInstance* Clone() const;

    // GetOwner can return nullptr, if "this" is an inner emitter of ParticleLayer
    ParticleEffectComponent* GetOwner() const;

private:
    ParticleEffectComponent* owner = nullptr;
    RefPtr<ParticleEmitter> emitter;
    FilePath filePath;
    Vector3 spawnPosition;

    DAVA_VIRTUAL_REFLECTION(ParticleEmitterInstance, BaseObject);
};

inline ParticleEmitter* ParticleEmitterInstance::GetEmitter() const
{
    return emitter.Get();
}

inline const FilePath& ParticleEmitterInstance::GetFilePath() const
{
    return filePath;
}

inline const Vector3& ParticleEmitterInstance::GetSpawnPosition() const
{
    return spawnPosition;
}

inline ParticleEffectComponent* ParticleEmitterInstance::GetOwner() const
{
    return owner;
}

inline void ParticleEmitterInstance::SetEmitter(ParticleEmitter* _emitter)
{
    emitter.Set(_emitter);
}

inline void ParticleEmitterInstance::SetFilePath(const FilePath& _filePath)
{
    filePath = _filePath;
}

inline void ParticleEmitterInstance::SetSpawnPosition(const Vector3& _position)
{
    spawnPosition = _position;
}

inline void ParticleEmitterInstance::SetOwner(ParticleEffectComponent* owner_)
{
    owner = owner_;
}
}
