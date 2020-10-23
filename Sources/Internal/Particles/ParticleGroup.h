#pragma once

#include "ParticleEmitter.h"
#include "ParticleLayer.h"
#include "Particle.h"
#include "Render/Material/NMaterial.h"

namespace DAVA
{
struct StripeNode
{
    float32 lifeime = 0.0f;
    Vector3 position = {};
    Vector3 speed = {};
    float32 distanceFromBase = 0.0f;
    float32 distanceFromPrevNode = 0.0f;

    StripeNode(float32 lifetime_, Vector3 position_, Vector3 speed_, float32 distanceFromBase_, float32 distanceFromPrevNode_)
        : lifeime(lifetime_)
        , position(position_)
        , speed(speed_)
        , distanceFromBase(distanceFromBase_)
        , distanceFromPrevNode(distanceFromPrevNode_)
    {
    }

    StripeNode() = default;
};

struct StripeData
{
    List<StripeNode> stripeNodes; // List of stripe control points.
    StripeNode baseNode;
    Vector3 inheritPositionOffset = {};
    bool isActive = true;
    float32 uvOffset = 0.0f;
    float32 prevBaseLen = 0.0f;
};

struct ParticleGroup
{
    ParticleEmitter* emitter = nullptr;
    ParticleLayer* layer = nullptr;
    NMaterial* material = nullptr;
    Particle* head = nullptr;

    Vector3 spawnPosition;

    int32 positionSource = 0;
    int32 activeParticleCount = 0;

    float32 time = 0.0f;
    float32 loopStartTime = 0.0f;
    float32 loopLayerStartTime = 0.0f;
    float32 loopDuration = 0.0f;
    float32 particlesToGenerate = 0.0f;

    uint16 particlesGenerated = 0;

    bool finishingGroup = false;
    bool visibleLod = true;

    StripeData stripe;
};

struct ParentInfo
{
    Vector3 position;
    Vector2 size;
};

struct ParticleEffectData
{
    Vector<ParentInfo> infoSources;
    List<ParticleGroup> groups;
};
}
