#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class ParticleForce;
class Vector3;
class Entity;
struct Particle;

class ParticleForces
{
public:
    static void ApplyForce(const ParticleForce* force, Vector3& velocity, Vector3& position, float32 dt, float32 particleOverLife, float32 layerOverLife, const Vector3& down, Particle* particle, const Vector3& prevPosition, const Vector3& forcePosition);
};

class ParticleForcesUtils
{
public:
    static void GenerateNoise();
    static void GenerateSphereRandomVectors();
};
}