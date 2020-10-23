#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class WaveSystem;
class WaveComponent : public Component
{
protected:
    virtual ~WaveComponent();

public:
    WaveComponent();
    WaveComponent(float32 waveAmlitude, float32 waveLenght, float32 waveSpeed, float32 dampingRatio, float32 influenceDistance);

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Trigger();

    inline const float32& GetWaveAmplitude() const;
    inline const float32& GetWaveLenght() const;
    inline const float32& GetWaveSpeed() const;
    inline const float32& GetDampingRatio() const;
    inline const float32& GetInfluenceRadius() const;

    inline void SetWaveAmplitude(const float32& amplitude);
    inline void SetWaveLenght(const float32& lenght);
    inline void SetWaveSpeed(const float32& speed);
    inline void SetDampingRatio(const float32& damping);
    inline void SetInfluenceRadius(const float32& infRadius);

protected:
    float32 amplitude;
    float32 lenght;
    float32 speed;
    float32 damping;
    float32 infRadius;

    DAVA_VIRTUAL_REFLECTION(WaveComponent, Component);
};

inline const float32& WaveComponent::GetWaveAmplitude() const
{
    return amplitude;
}

inline const float32& WaveComponent::GetWaveLenght() const
{
    return lenght;
}

inline const float32& WaveComponent::GetWaveSpeed() const
{
    return speed;
}

inline const float32& WaveComponent::GetDampingRatio() const
{
    return damping;
}

inline const float32& WaveComponent::GetInfluenceRadius() const
{
    return infRadius;
}

inline void WaveComponent::SetWaveAmplitude(const float32& _amplitude)
{
    amplitude = _amplitude;
}

inline void WaveComponent::SetWaveLenght(const float32& _lenght)
{
    lenght = _lenght;
}

inline void WaveComponent::SetWaveSpeed(const float32& _speed)
{
    speed = _speed;
}

inline void WaveComponent::SetDampingRatio(const float32& _damping)
{
    damping = _damping;
}

inline void WaveComponent::SetInfluenceRadius(const float32& _infRadius)
{
    infRadius = _infRadius;
}
};
