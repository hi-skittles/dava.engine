#pragma once

#include "Base/BaseObject.h"
#include "Particles/ParticlePropertyLine.h"

#include "Reflection/Reflection.h"

namespace DAVA
{
// Particle Force class is needed to store Particle Force data.
class ParticleForceSimplified : public BaseObject
{
public:
    ParticleForceSimplified() = default;
    ParticleForceSimplified(RefPtr<PropertyLine<Vector3>> force, RefPtr<PropertyLine<float32>> forceOverLife);

    ParticleForceSimplified* Clone();
    void GetModifableLines(List<ModifiablePropertyLineBase*>& modifiables);

    RefPtr<PropertyLine<Vector3>> force;
    RefPtr<PropertyLine<float32>> forceOverLife;

    DAVA_VIRTUAL_REFLECTION(ParticleForceSimplified, BaseObject);
};
};
