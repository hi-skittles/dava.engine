#include "Particles/ParticleForce.h"

#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(ParticleForce)
{
    ReflectionRegistrator<ParticleForce>::Begin()
    .End();
}

ParticleForce::ParticleForce(ParticleLayer* parent)
    : parentLayer(parent)
{
}

ParticleForce* ParticleForce::Clone()
{
    ParticleForce* dst = new ParticleForce(parentLayer);
    if (forcePowerLine != nullptr)
    {
        dst->forcePowerLine = forcePowerLine->Clone();
        dst->forcePowerLine->Release();
    }
    if (turbulenceLine != nullptr)
    {
        dst->turbulenceLine = turbulenceLine->Clone();
        dst->turbulenceLine->Release();
    }
    dst->direction = direction;
    dst->isActive = isActive;
    dst->timingType = timingType;
    dst->forceName = forceName;
    dst->shape = shape;
    dst->type = type;
    dst->parentLayer = parentLayer;
    dst->position = position;
    dst->rotation = rotation;
    dst->isInfinityRange = isInfinityRange;
    dst->killParticles = killParticles;
    dst->normalAsReflectionVector = normalAsReflectionVector;
    dst->randomizeReflectionForce = randomizeReflectionForce;
    dst->worldAlign = worldAlign;
    dst->reflectionPercent = reflectionPercent;
    dst->rndReflectionForceMin = rndReflectionForceMin;
    dst->rndReflectionForceMax = rndReflectionForceMax;
    dst->velocityThreshold = velocityThreshold;
    dst->pointGravityUseRandomPointsOnSphere = pointGravityUseRandomPointsOnSphere;
    dst->isGlobal = isGlobal;
    dst->boxSize = boxSize;
    dst->forcePower = forcePower;
    dst->radius = radius;
    dst->windFrequency = windFrequency;
    dst->windTurbulenceFrequency = windTurbulenceFrequency;
    dst->windTurbulence = windTurbulence;
    dst->backwardTurbulenceProbability = backwardTurbulenceProbability;
    dst->windBias = windBias;
    dst->pointGravityRadius = pointGravityRadius;
    dst->planeScale = planeScale;
    dst->reflectionChaos = reflectionChaos;

    dst->startTime = startTime;
    dst->endTime = endTime;

    return dst;
}

void ParticleForce::GetModifableLines(List<ModifiablePropertyLineBase*>& modifiables)
{
    PropertyLineHelper::AddIfModifiable(forcePowerLine.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(turbulenceLine.Get(), modifiables);
}
}