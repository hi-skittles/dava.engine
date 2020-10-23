#pragma once

#include "Entity/Component.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Particles/ParticleGroup.h"
#include "Particles/ParticleRenderObject.h"
#include "Particles/ParticleEmitterInstance.h"
#include "Reflection/Reflection.h"
#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/Message.h"

namespace DAVA
{
class ModifiablePropertyLineBase;
class ParticleEffectComponent : public Component
{
    friend class ParticleEffectSystem;
    friend class UIParticles;

    static const uint32 PARTICLE_FLAGS_SERIALIZATION_CRITERIA = RenderObject::VISIBLE | RenderObject::VISIBLE_REFLECTION | RenderObject::VISIBLE_REFRACTION;

public:
    enum eState : uint32
    {
        STATE_PLAYING, //effect is playing
        STATE_STARTING, //effect is starting - on next system update it would be moved to playing state (RunEffect called)
        STATE_STOPPING, //effect is stopping - no new particle generation, still need to update and recalculate
        STATE_STOPPED //effect is completely stopped and removed from active lists
    };

    ParticleEffectComponent();
    ~ParticleEffectComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void Start();
    void Stop(bool isDeleteAllParticles = true);
    void Restart(bool isDeleteAllParticles = true);
    bool IsStopped();
    void Pause(bool isPaused = true);
    bool IsPaused();
    void Step(float32 delta);

    void StopAfterNRepeats(int32 numberOfRepeats);
    void StopWhenEmpty(bool value = true);
    float32 GetPlaybackSpeed();
    void SetPlaybackSpeed(float32 value);

    void SetDesiredLodLevel(int32 level);

    void SetPlaybackCompleteMessage(const Message& msg);

    /*externals stuff*/
    void SetExtertnalValue(const String& name, float32 value);
    float32 GetExternalValue(const String& name);
    Set<String> EnumerateVariables();
    void RebuildEffectModifiables();
    void RegisterModifiable(ModifiablePropertyLineBase* propertyLine);
    void UnRegisterModifiable(ModifiablePropertyLineBase* propertyLine);

    int32 GetActiveParticlesCount();

    void SetRenderObjectVisible(bool visible);

    /*sorting offset allowed in 0..31 range, 15 default, more - closer to camera*/
    void SetSortingOffset(uint32 offset);

    bool GetReflectionVisible() const;
    void SetReflectionVisible(bool visible);
    bool GetRefractionVisible() const;
    void SetRefractionVisible(bool visible);
    bool GetClippingVisible() const;
    void SetClippingVisible(bool visible);

    float32 GetStartFromTime() const;
    void SetStartFromTime(float32 time);

    inline eState GetAnimationState() const;
    inline ParticleRenderObject* GetRenderObject() const;

    void ReloadEmitters();

private:
    void ClearGroup(ParticleGroup& group);
    void ClearCurrentGroups();
    void SetGroupsFinishing();

    /*completion message stuff*/
    Message playbackComplete;

    /*externals setup*/
    MultiMap<String, ModifiablePropertyLineBase*> externalModifiables;
    Map<String, float32> externalValues;

    /*Emitters setup*/
    Vector<RefPtr<ParticleEmitterInstance>> emitterInstances;

    ParticleEffectData effectData;
    ParticleRenderObject* effectRenderObject;

    eState state = STATE_STOPPED;
    float32 effectDuration = 100.0f;
    float32 time = 0.0f;
    float32 playbackSpeed = 1.0f;
    uint32 currRepeatsCont = 0;
    uint32 repeatsCount = -1; // note that now it's really count - not depending if effect is stop when empty or by duration - it would be restarted if currRepeatsCount<repetsCount
    int32 desiredLodLevel = 1;
    int32 activeLodLevel = 1;
    float32 startFromTime = 0.0f;

    bool stopWhenEmpty = false; //if true effect is considered finished when no particles left, otherwise effect is considered finished if time>effectDuration
    bool clearOnRestart = true; // when effect is restarted repeatsCount
    bool isPaused = false;

public: //mostly editor commands
    uint32 GetEmittersCount() const;

    Vector3 GetSpawnPosition(int32 id) const;
    void SetSpawnPosition(int32 id, const Vector3& position);

    FilePath GetOriginalConfigPath(int32 id) const;
    void SetOriginalConfigPath(int32 id, const FilePath& filepath);

    int32 GetEmitterInstanceIndex(ParticleEmitterInstance* emitter) const;
    ParticleEmitterInstance* GetEmitterInstance(uint32 id) const;
    DAVA_DEPRECATED(ParticleEmitter* GetEmitter(uint32 id) const);

    void AddEmitterInstance(ParticleEmitter* emitter);
    void AddEmitterInstance(ParticleEmitterInstance* emitter);
    void InsertEmitterInstanceAt(ParticleEmitterInstance* emitter, uint32 position);
    void RemoveEmitterInstance(ParticleEmitterInstance* emitter);

    float32 GetCurrTime();

    /*statistics for editor*/
    int32 GetLayerActiveParticlesCount(ParticleLayer* layer);
    float32 GetLayerActiveParticlesSquare(ParticleLayer* layer);

public:
    uint32 loadedVersion = 0;
    void CollapseOldEffect(SerializationContext* serializationContext);

    DAVA_VIRTUAL_REFLECTION(ParticleEffectComponent, Component);
};

inline float32 ParticleEffectComponent::GetStartFromTime() const
{
    return startFromTime;
}

inline void ParticleEffectComponent::SetStartFromTime(float32 time)
{
    startFromTime = Clamp(time, 0.0f, effectDuration);
}

ParticleEffectComponent::eState ParticleEffectComponent::GetAnimationState() const
{
    return state;
}

ParticleRenderObject* ParticleEffectComponent::GetRenderObject() const
{
    return effectRenderObject;
}
}
