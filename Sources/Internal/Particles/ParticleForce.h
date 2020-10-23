#pragma once

#include "Base/BaseObject.h"

#include "Math/Vector.h"
#include "Math/Matrix4.h"
#include "Particles/ParticlePropertyLine.h"

#include "Reflection/Reflection.h"

namespace DAVA
{
struct ParticleLayer;

class ParticleForce : public BaseObject
{
public:
    enum class eShape : uint8
    {
        BOX,
        SPHERE
    };

    enum class eTimingType : uint8
    {
        CONSTANT,
        OVER_LAYER_LIFE,
        OVER_PARTICLE_LIFE,
        SECONDS_PARTICLE_LIFE
    } timingType = eTimingType::CONSTANT;

    enum class eType : uint8
    {
        DRAG_FORCE = 0, // Also force priority.
        WIND,
        VORTEX,
        GRAVITY,
        POINT_GRAVITY,
        PLANE_COLLISION
    } type = eType::DRAG_FORCE;

    RefPtr<PropertyLine<Vector3>> forcePowerLine;
    RefPtr<PropertyLine<float32>> turbulenceLine;

    Vector3 position;
    Vector3 worldPosition;
    Vector3 rotation;
    Vector3 direction{ 0.0f, 0.0f, 1.0f };
    Vector3 forcePower{ 1.0f, 1.0f, 1.0f };

    float32 windFrequency = 0.0f;
    float32 windTurbulenceFrequency = 0.0f;
    float32 windBias = 1.0f;

    float32 windTurbulence = 0.0f;
    float32 pointGravityRadius = 1.0f;
    float32 planeScale = 5.0f; // Editor only.
    float32 reflectionChaos = 0.0f;
    float32 rndReflectionForceMin = 1.0f;
    float32 rndReflectionForceMax = 1.1f;
    float32 velocityThreshold = 0.3f;
    float32 startTime = 0.0f;
    float32 endTime = 15.0f;

    uint32 backwardTurbulenceProbability = 0;
    uint32 reflectionPercent = 100;

    String forceName = "Particle Force";
    bool isActive = true;
    bool isInfinityRange = true;
    bool pointGravityUseRandomPointsOnSphere = false;
    bool isGlobal = false;
    bool killParticles = false;
    bool normalAsReflectionVector = true;
    bool randomizeReflectionForce = false;
    bool worldAlign = true;

public:
    ParticleForce(ParticleLayer* parent);
    ParticleForce* Clone();
    void GetModifableLines(List<ModifiablePropertyLineBase*>& modifiables);
    void SetRadius(float32 radius);
    void SetBoxSize(const Vector3& boxSize);
    float32 GetRadius() const;
    const Vector3& GetBoxSize() const;
    const Vector3& GetHalfBoxSize() const;
    float32 GetSquaredRadius() const;
    void SetShape(eShape shape);
    eShape GetShape() const;
    bool CanAlterPosition() const;

    DAVA_VIRTUAL_REFLECTION(ParticleForce, BaseObject);

private:
    Vector3 boxSize{ 1.0f, 1.0f, 1.0f };
    Vector3 halfBoxSize{ 0.5f, 0.5f, 0.5f };
    float32 squaredRadius = 0.75f; // For default box with 0.5f edges.
    float32 radius = 1.0f;
    eShape shape = eShape::BOX;
    ParticleLayer* parentLayer = nullptr;
};

inline void ParticleForce::SetRadius(float32 radius_)
{
    radius = radius_;
    if (shape == eShape::SPHERE)
        squaredRadius = radius * radius;
}

inline void ParticleForce::SetBoxSize(const Vector3& boxSize_)
{
    boxSize = boxSize_;
    halfBoxSize = boxSize * 0.5f;
    if (shape == eShape::BOX)
        squaredRadius = halfBoxSize.DotProduct(halfBoxSize);
}

inline float32 ParticleForce::GetRadius() const
{
    return radius;
}

inline const Vector3& ParticleForce::GetBoxSize() const
{
    return boxSize;
}

inline const Vector3& ParticleForce::GetHalfBoxSize() const
{
    return halfBoxSize;
}

inline float32 ParticleForce::GetSquaredRadius() const
{
    return squaredRadius;
}

inline void ParticleForce::SetShape(ParticleForce::eShape shape_)
{
    shape = shape_;
    if (shape == eShape::BOX)
    {
        halfBoxSize = boxSize * 0.5f;
        squaredRadius = boxSize.DotProduct(halfBoxSize);
    }
    else if (shape == eShape::SPHERE)
        squaredRadius = radius * radius;
}

inline ParticleForce::eShape ParticleForce::GetShape() const
{
    return shape;
}

inline bool ParticleForce::CanAlterPosition() const
{
    return type == ParticleForce::eType::POINT_GRAVITY || type == ParticleForce::eType::PLANE_COLLISION || type == ParticleForce::eType::WIND;
}
}
