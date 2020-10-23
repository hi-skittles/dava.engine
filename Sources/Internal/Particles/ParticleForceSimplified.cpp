#include "ParticleForceSimplified.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(ParticleForceSimplified)
{
    ReflectionRegistrator<ParticleForceSimplified>::Begin()
    .End();
}

// Particle Force class is needed to store Particle Force data.
ParticleForceSimplified::ParticleForceSimplified(RefPtr<PropertyLine<Vector3>> force_, RefPtr<PropertyLine<float32>> forceOverLife_)
    : force(force_)
    , forceOverLife(forceOverLife_)
{
}

ParticleForceSimplified* ParticleForceSimplified::Clone()
{
    ParticleForceSimplified* dst = new ParticleForceSimplified();
    if (force)
    {
        dst->force = force->Clone();
        dst->force->Release();
    }
    if (forceOverLife)
    {
        dst->forceOverLife = forceOverLife->Clone();
        dst->forceOverLife->Release();
    }
    return dst;
}

void ParticleForceSimplified::GetModifableLines(List<ModifiablePropertyLineBase*>& modifiables)
{
    PropertyLineHelper::AddIfModifiable(force.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(forceOverLife.Get(), modifiables);
}
}
