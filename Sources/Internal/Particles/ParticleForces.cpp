#include "Particles/ParticleForces.h"

#include <random>
#include <chrono>

#include "Particles/Particle.h"
#include "Particles/ParticleForce.h"
#include "Math/MathHelpers.h"
#include "Math/Noise.h"
#include "Scene3D/Entity.h"

namespace DAVA
{
namespace ParticleForcesDetails
{
const int32 noiseWidth = 57;
const int32 noiseHeight = 57;
Array<Array<Vector3, noiseWidth>, noiseHeight> noise;

const uint32 sphereRandomVectorsSize = 1024;
Array<Vector3, sphereRandomVectorsSize> sphereRandomVectors;

template <typename T>
T GetValue(const ParticleForce* force, float32 particleOverLife, float32 layerOverLife, float32 particleLife, PropertyLine<T>* line, T value)
{
    if (line == nullptr)
        return value;
    switch (force->timingType)
    {
    case ParticleForce::eTimingType::CONSTANT:
        return value;
    case ParticleForce::eTimingType::OVER_PARTICLE_LIFE:
        return line->GetValue(particleOverLife);
    case ParticleForce::eTimingType::OVER_LAYER_LIFE:
        return line->GetValue(layerOverLife);
    case ParticleForce::eTimingType::SECONDS_PARTICLE_LIFE:
        return line->GetValue(particleLife);
    default:
        return value;
    }
    return value;
}

Vector3 GetNoiseValue(float32 particleOverLife, float32 frequency, uint32 clampedIndex)
{
    float32 indexUnclamped = particleOverLife * noiseWidth * frequency + clampedIndex;
    float32 intPart = 0.0f;
    float32 fractPart = std::modf(particleOverLife * noiseWidth * frequency + clampedIndex, &intPart);
    uint32 xindex = static_cast<uint32>(intPart);

    xindex %= noiseWidth;
    uint32 yindex = clampedIndex % noiseHeight;
    uint32 nextIndex = (xindex + 1) % noiseWidth;
    Vector3 t1 = noise[xindex][yindex];
    Vector3 t2 = noise[nextIndex][yindex];
    return Lerp(t1, t2, fractPart);
}

inline void KillParticle(Particle* particle)
{
    particle->life = particle->lifeTime + 0.1f;
}

inline void KillParticlePlaneCollision(const ParticleForce* force, Particle* particle, Vector3& effectSpaceVelocity)
{
    if (force->killParticles)
        KillParticle(particle);
    else
        effectSpaceVelocity = Vector3::Zero;
}

inline bool IsPositionInForceShape(const ParticleForce* force, const Vector3& particlePosition, const Vector3& forcePosition)
{
    if (force->isInfinityRange || force->type == ParticleForce::eType::GRAVITY)
        return true;

    if (force->GetShape() == ParticleForce::eShape::BOX)
    {
        AABBox3 box(forcePosition - force->GetHalfBoxSize(), forcePosition + force->GetHalfBoxSize());
        if (box.IsInside(particlePosition))
            return true;
    }
    else if (force->GetShape() == ParticleForce::eShape::SPHERE)
    {
        if ((forcePosition - particlePosition).SquareLength() <= force->GetSquaredRadius())
            return true;
    }
    return false;
}

void ApplyDragForce(const ParticleForce* force, Vector3& velocity, const Vector3& position, float32 dt, float32 particleOverLife, float32 layerOverLife, const Particle* particle, const Vector3& forcePosition)
{
    Vector3 forceStrength = GetValue(force, particleOverLife, layerOverLife, particle->life, force->forcePowerLine.Get(), force->forcePower) * dt;
    Vector3 v(Max(Vector3::Zero, 1.0f - forceStrength));
    velocity *= v;
}

void ApplyVortex(const ParticleForce* force, Vector3& velocity, const Vector3& position, float32 dt, float32 particleOverLife, float32 layerOverLife, const Particle* particle, const Vector3& forcePosition)
{
    Vector3 forceDir = (position - forcePosition).CrossProduct(force->direction);
    float32 len = forceDir.SquareLength();
    if (len > 0.0f)
    {
        float32 d = 1.0f / std::sqrt(len);
        forceDir *= d;
    }
    Vector3 forceStrength = GetValue(force, particleOverLife, layerOverLife, particle->life, force->forcePowerLine.Get(), force->forcePower) * dt;
    velocity += forceStrength * forceDir;
}

void ApplyGravity(const ParticleForce* force, Vector3& velocity, const Vector3& down, float32 dt, float32 particleOverLife, float32 layerOverLife, const Particle* particle)
{
    velocity += down * GetValue(force, particleOverLife, layerOverLife, particle->life, force->forcePowerLine.Get(), force->forcePower).x * dt;
}

void ApplyWind(const ParticleForce* force, Vector3& velocity, Vector3& position, float32 dt, float32 particleOverLife, float32 layerOverLife, const Particle* particle, const Vector3& forcePosition)
{
    static const float32 windScale = 100.0f; // Artiom request.

    uintptr_t partInd = reinterpret_cast<uintptr_t>(particle);
    uint64 particleIndex = static_cast<uint64>(partInd);
    Vector3 turbulence;

    uint32 clampedIndex = particleIndex % noiseWidth;
    float32 windMultiplier = 1.0f;
    float32 tubulencePower = GetValue(force, particleOverLife, layerOverLife, particle->life, force->turbulenceLine.Get(), force->windTurbulence);
    if (Abs(tubulencePower) > EPSILON)
    {
        turbulence = GetNoiseValue(particleOverLife, force->windTurbulenceFrequency, clampedIndex);
        if ((100 - force->backwardTurbulenceProbability) > clampedIndex % 100)
        {
            float32 dot = Normalize(force->direction).DotProduct(Normalize(turbulence));
            if (dot < 0.0f)
                turbulence *= -1.0f;
        }
        turbulence *= tubulencePower * dt;
        position += turbulence;
    }
    if (Abs(force->windFrequency) > EPSILON)
    {
        float32 noiseVal = GetNoiseValue(particleOverLife, force->windFrequency, clampedIndex).x;
        windMultiplier = noiseVal + force->windBias;
    }
    Vector3 forceStrength = GetValue(force, particleOverLife, layerOverLife, particle->life, force->forcePowerLine.Get(), force->forcePower) * dt;
    velocity += force->direction * dt * windMultiplier * forceStrength.x * windScale;
}

void ApplyPointGravity(const ParticleForce* force, Vector3& velocity, Vector3& position, float32 dt, float32 particleOverLife, float32 layerOverLife, Particle* particle, const Vector3& forcePosition)
{
    Vector3 toCenter = forcePosition - position;
    float32 sqrToCenterDist = toCenter.SquareLength();
    if (sqrToCenterDist > 0)
        toCenter /= sqrt(sqrToCenterDist);

    Vector3 forceDirection = toCenter;
    if (force->pointGravityUseRandomPointsOnSphere)
    {
        uintptr_t partInd = reinterpret_cast<uintptr_t>(particle);
        size_t particleIndex = static_cast<size_t>(partInd);
        particleIndex %= sphereRandomVectorsSize;
        Vector3 forcePositionModified = forcePosition + sphereRandomVectors[particleIndex] * force->pointGravityRadius;
        forceDirection = forcePositionModified - position;
        float32 sqrDistToTarget = forceDirection.SquareLength();
        if (sqrDistToTarget > 0)
            forceDirection /= sqrt(sqrDistToTarget);
    }

    Vector3 forceStrength = GetValue(force, particleOverLife, layerOverLife, particle->life, force->forcePowerLine.Get(), force->forcePower) * dt;
    if (sqrToCenterDist > force->pointGravityRadius * force->pointGravityRadius)
        velocity += forceDirection * forceStrength;
    else
    {
        if (force->killParticles)
            KillParticle(particle);
        else
            position = forcePosition - force->pointGravityRadius * toCenter;
    }
}

void ApplyPlaneCollision(const ParticleForce* force, Vector3& velocity, Vector3& position, Particle* particle, const Vector3& prevPosition, const Vector3& forcePosition)
{
    Vector3 normal = Normalize(force->direction);
    Vector3 a = prevPosition - forcePosition;
    Vector3 b = position - forcePosition;
    float32 bProj = b.DotProduct(normal);
    float32 aProj = a.DotProduct(normal);
    if (bProj <= 0 && aProj > 0)
    {
        if (velocity.SquareLength() < force->velocityThreshold * force->velocityThreshold)
        {
            KillParticlePlaneCollision(force, particle, velocity);
            return;
        }

        Vector3 dir = prevPosition - position;
        float32 abProj = Abs(dir.DotProduct(normal));
        if (abProj < EPSILON)
            return;
        position = position + dir * (-bProj) / abProj;

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int32> uniInt(0, 99);
        bool reflectParticle = static_cast<uint32>(uniInt(rng)) < force->reflectionPercent;
        if (reflectParticle)
        {
            Vector3 newVel;
            if (force->normalAsReflectionVector)
                newVel = (normal * velocity.Length()); // Artiom request.
            else
                newVel = Reflect(velocity, normal);

            if (Abs(force->reflectionChaos) > EPSILON)
            {
                std::uniform_real_distribution<float32> uni(-DegToRad(force->reflectionChaos), DegToRad(force->reflectionChaos));
                Quaternion q = Quaternion::MakeRotationFastX(uni(rng)) * Quaternion::MakeRotationFastY(uni(rng)) * Quaternion::MakeRotationFastZ(uni(rng));
                newVel = q.ApplyToVectorFast(newVel);
                if (newVel.DotProduct(normal) < 0)
                    newVel = -newVel;
            }
            velocity = newVel * force->forcePower;
            if (force->randomizeReflectionForce)
                velocity *= std::uniform_real_distribution<float32>(force->rndReflectionForceMin, force->rndReflectionForceMax)(rng);
        }
        else
            KillParticlePlaneCollision(force, particle, velocity);
    }
    else if (bProj < 0.0f && aProj < 0.0f)
        KillParticlePlaneCollision(force, particle, velocity);
}
}

void ParticleForces::ApplyForce(const ParticleForce* force, Vector3& velocity, Vector3& position, float32 dt, float32 particleOverLife, float32 layerOverLife, const Vector3& down, Particle* particle, const Vector3& prevPosition, const Vector3& forcePosition)
{
    using ForceType = ParticleForce::eType;

    if (!force->isActive || !ParticleForcesDetails::IsPositionInForceShape(force, position, forcePosition))
        return;
    switch (force->type)
    {
    case ForceType::DRAG_FORCE:
        ParticleForcesDetails::ApplyDragForce(force, velocity, position, dt, particleOverLife, layerOverLife, particle, forcePosition);
        break;
    case ForceType::VORTEX:
        ParticleForcesDetails::ApplyVortex(force, velocity, position, dt, particleOverLife, layerOverLife, particle, forcePosition);
        break;
    case ForceType::GRAVITY:
        ParticleForcesDetails::ApplyGravity(force, velocity, down, dt, particleOverLife, layerOverLife, particle);
        break;
    case ForceType::WIND:
        ParticleForcesDetails::ApplyWind(force, velocity, position, dt, particleOverLife, layerOverLife, particle, forcePosition);
        break;
    case ForceType::POINT_GRAVITY:
        ParticleForcesDetails::ApplyPointGravity(force, velocity, position, dt, particleOverLife, layerOverLife, particle, forcePosition);
        break;
    case ForceType::PLANE_COLLISION:
        ParticleForcesDetails::ApplyPlaneCollision(force, velocity, position, particle, prevPosition, forcePosition);
        break;
    default:
        DVASSERT(false, "Unsupported force.");
        break;
    }
}

void ParticleForcesUtils::GenerateSphereRandomVectors()
{
    uint32 seed = static_cast<uint32>(std::chrono::system_clock::now().time_since_epoch().count());
    std::mt19937 generator(seed);

    std::uniform_real_distribution<float32> uinform01(0.0f, 1.0f);
    for (uint32 i = 0; i < ParticleForcesDetails::sphereRandomVectorsSize; ++i)
    {
        float32 theta = 2.0f * PI * uinform01(generator);
        float32 cosPhi = 1.0f - 2.0f * uinform01(generator);
        float32 phi = std::acos(cosPhi);
        float32 sinPhi = std::sin(phi);
        ParticleForcesDetails::sphereRandomVectors[i] = { sinPhi * std::cos(theta), sinPhi * std::sin(theta), cosPhi };
    }
}

void ParticleForcesUtils::GenerateNoise()
{
    float32 xFactor = 2.0f / (ParticleForcesDetails::noiseWidth - 1.0f);
    float32 yFactor = 2.0f / (ParticleForcesDetails::noiseHeight - 1.0f);
    for (int32 i = 0; i < ParticleForcesDetails::noiseWidth; ++i)
    {
        Vector2 q(i * xFactor, 0);
        for (int32 j = 0; j < ParticleForcesDetails::noiseHeight; ++j)
        {
            q.y = j * yFactor;
            ParticleForcesDetails::noise[i][j] = Generate2OctavesPerlin(q);
        }
    }
}
}
