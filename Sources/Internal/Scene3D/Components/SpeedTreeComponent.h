#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class SpeedTreeUpdateSystem;
class SpeedTreeComponent : public Component
{
protected:
    virtual ~SpeedTreeComponent();

public:
    SpeedTreeComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    inline float32 GetTrunkOscillationAmplitude() const;
    inline float32 GetTrunkOscillationSpring() const;
    inline float32 GetTrunkOscillationDamping() const;
    inline float32 GetLeafsOscillationApmlitude() const;
    inline float32 GetLeafsOscillationSpeed() const;
    inline int32 GetMaxAnimatedLOD() const;

    inline void SetTrunkOscillationAmplitude(const float32& amplitude);
    inline void SetTrunkOscillationSpring(const float32& spring);
    inline void SetTrunkOscillationDamping(const float32& damping);
    inline void SetLeafsOscillationApmlitude(const float32& amplitude);
    inline void SetLeafsOscillationSpeed(const float32& speed);
    void SetMaxAnimatedLOD(const int32& lodIndex);

protected:
    float32 trunkOscillationAmplitude;
    float32 trunkOscillationSpring;
    float32 trunkOscillationDamping;
    float32 leafsOscillationAmplitude;
    float32 leafsOscillationSpeed;
    int32 maxAnimatedLOD;

    //runtime properties
    Matrix4 wtInvMx;
    Vector3 wtPosition;

    Vector2 oscVelocity;
    Vector2 oscOffset;
    float32 leafTime;

    DAVA_VIRTUAL_REFLECTION(SpeedTreeComponent, Component);

    friend class SpeedTreeUpdateSystem;
};

inline float32 SpeedTreeComponent::GetTrunkOscillationAmplitude() const
{
    return trunkOscillationAmplitude;
}

inline float32 SpeedTreeComponent::GetTrunkOscillationSpring() const
{
    return trunkOscillationSpring;
}

inline float32 SpeedTreeComponent::GetTrunkOscillationDamping() const
{
    return trunkOscillationDamping;
}

inline float32 SpeedTreeComponent::GetLeafsOscillationApmlitude() const
{
    return leafsOscillationAmplitude;
}

inline float32 SpeedTreeComponent::GetLeafsOscillationSpeed() const
{
    return leafsOscillationSpeed;
}

inline int32 SpeedTreeComponent::GetMaxAnimatedLOD() const
{
    return maxAnimatedLOD;
}

inline void SpeedTreeComponent::SetTrunkOscillationAmplitude(const float32& amplitude)
{
    trunkOscillationAmplitude = amplitude;
}

inline void SpeedTreeComponent::SetTrunkOscillationSpring(const float32& spring)
{
    trunkOscillationSpring = Clamp(spring, .5f, 20.f);
}

inline void SpeedTreeComponent::SetTrunkOscillationDamping(const float32& damping)
{
    trunkOscillationDamping = Clamp(damping, .0f, 20.f);
}

inline void SpeedTreeComponent::SetLeafsOscillationApmlitude(const float32& amplitude)
{
    leafsOscillationAmplitude = amplitude;
}

inline void SpeedTreeComponent::SetLeafsOscillationSpeed(const float32& speed)
{
    leafsOscillationSpeed = speed;
}
}
