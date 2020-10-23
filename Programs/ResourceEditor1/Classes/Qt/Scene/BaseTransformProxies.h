#pragma once

#include "Classes/Selection/Selectable.h"
#include <Reflection/Reflection.h>
#include <Particles/ParticleForce.h>

namespace DAVA
{
class ParticleEmitterInstance;
}

class EntityTransformProxy : public Selectable::TransformProxy
{
public:
    const DAVA::Matrix4& GetWorldTransform(const DAVA::Any& object) override;
    const DAVA::Matrix4& GetLocalTransform(const DAVA::Any& object) override;
    void SetLocalTransform(DAVA::Any& object, const DAVA::Matrix4& matrix) override;
    bool SupportsTransformType(const DAVA::Any& object, Selectable::TransformType) const override;
    bool TransformDependsFromObject(const DAVA::Any& dependant, const DAVA::Any& dependsOn) const override;
};

class EmitterTransformProxy : public Selectable::TransformProxy
{
public:
    const DAVA::Matrix4& GetWorldTransform(const DAVA::Any& object) override;
    const DAVA::Matrix4& GetLocalTransform(const DAVA::Any& object) override;
    void SetLocalTransform(DAVA::Any& object, const DAVA::Matrix4& matrix) override;
    bool SupportsTransformType(const DAVA::Any& object, Selectable::TransformType) const override;
    bool TransformDependsFromObject(const DAVA::Any& dependant, const DAVA::Any& dependsOn) const override;
};

class ParticleForceTransformProxy : public Selectable::TransformProxy
{
public:
    const DAVA::Matrix4& GetWorldTransform(const DAVA::Any& object) override;
    const DAVA::Matrix4& GetLocalTransform(const DAVA::Any& object) override;
    void SetLocalTransform(DAVA::Any& object, const DAVA::Matrix4& matrix) override;
    bool SupportsTransformType(const DAVA::Any& object, Selectable::TransformType) const override;
    bool TransformDependsFromObject(const DAVA::Any& dependant, const DAVA::Any& dependsOn) const override;

private:
    DAVA::ParticleEmitterInstance* GetEmitterInstance(DAVA::ParticleForce* force) const;
};
