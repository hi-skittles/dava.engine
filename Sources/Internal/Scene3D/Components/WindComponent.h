#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"
#include "Scene3D/Entity.h"

namespace DAVA
{
class WindSystem;
class SerializationContext;
class WindComponent : public Component
{
protected:
    virtual ~WindComponent();

public:
    WindComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    Vector3 GetDirection() const;

    inline const AABBox3& GetInfluenceBBox() const;
    inline const float32& GetWindForce() const;
    inline const float32& GetWindSpeed() const;

    inline void SetInfluenceBBox(const AABBox3& bbox);
    inline void SetWindForce(const float32& force);
    inline void SetWindSpeed(const float32& speed);

protected:
    AABBox3 influenceBbox;
    float32 windForce;
    float32 windSpeed;

    DAVA_VIRTUAL_REFLECTION(WindComponent, Component);
};

inline const AABBox3& WindComponent::GetInfluenceBBox() const
{
    return influenceBbox;
}

inline const float32& WindComponent::GetWindForce() const
{
    return windForce;
}

inline const float32& WindComponent::GetWindSpeed() const
{
    return windSpeed;
}

inline void WindComponent::SetInfluenceBBox(const AABBox3& bbox)
{
    influenceBbox = bbox;
}

inline void WindComponent::SetWindForce(const float32& force)
{
    windForce = force;
}

inline void WindComponent::SetWindSpeed(const float32& speed)
{
    windSpeed = speed;
}
}
