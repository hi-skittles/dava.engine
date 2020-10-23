#pragma once

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"
#include "Scene3D/Components/ParticleEffectComponent.h"

namespace DAVA
{
class Component;
class ParticleForce;

class ParticleEffectSystem : public SceneSystem
{
    friend class ParticleEffectComponent;
    friend class UIParticles;

public:
    struct MaterialData
    {
        Texture* texture = nullptr;
        Texture* flowmap = nullptr;
        Texture* noise = nullptr;
        Texture* alphaRemapTexture = nullptr;
        eBlending blending = BLENDING_ALPHABLEND;
        uint64 layerId = 1;
        bool enableFog = false;
        bool enableFrameBlend = false;
        bool enableFlow = false;
        bool enableFlowAnimation = false;
        bool enableNoise = false;
        bool isNoiseAffectFlow = false;
        bool useFresnelToAlpha = false;
        bool enableAlphaRemap = false;
        bool usePerspectiveMapping = false;
        bool useThreePointGradient = false;

        bool operator==(const MaterialData& rhs)
        {
            bool isEqualByGradient = useThreePointGradient == rhs.useThreePointGradient;
            if (isEqualByGradient && useThreePointGradient)
                isEqualByGradient = layerId == rhs.layerId;

            return texture == rhs.texture
            && enableFog == rhs.enableFog
            && enableFrameBlend == rhs.enableFrameBlend
            && flowmap == rhs.flowmap
            && enableFlow == rhs.enableFlow
            && enableFlowAnimation == rhs.enableFlowAnimation
            && enableNoise == rhs.enableNoise
            && isNoiseAffectFlow == rhs.isNoiseAffectFlow
            && noise == rhs.noise
            && useFresnelToAlpha == rhs.useFresnelToAlpha
            && blending == rhs.blending
            && enableAlphaRemap == rhs.enableAlphaRemap
            && alphaRemapTexture == rhs.alphaRemapTexture
            && usePerspectiveMapping == rhs.usePerspectiveMapping
            && isEqualByGradient;
        }
    };

    ParticleEffectSystem(Scene* scene, bool is2DMode = false);

    ~ParticleEffectSystem();
    void Process(float32 timeElapsed) override;
    void ImmediateEvent(Component* component, uint32 event) override;

    void AddEntity(Entity* entity) override;
    void AddComponent(Entity* entity, Component* component) override;

    void RemoveEntity(Entity* entity) override;
    void RemoveComponent(Entity* entity, Component* component) override;
    void PrepareForRemove() override;

    void SetGlobalMaterial(NMaterial* material);
    void SetGlobalExtertnalValue(const String& name, float32 value);
    float32 GetGlobalExternalValue(const String& name);
    Map<String, float32> GetGlobalExternals();

    inline void SetAllowLodDegrade(bool allowDegrade);
    inline bool GetAllowLodDegrade() const;

    inline const Vector<std::pair<MaterialData, NMaterial*>>& GetMaterialInstances() const;

    void PrebuildMaterials(ParticleEffectComponent* component);

protected:
    void RunEffect(ParticleEffectComponent* effect);
    void AddToActive(ParticleEffectComponent* effect);
    void RemoveFromActive(ParticleEffectComponent* effect);

    void UpdateActiveLod(ParticleEffectComponent* effect);
    void UpdateEffect(ParticleEffectComponent* effect, float32 deltaTime, float32 shortEffectTime);
    Particle* GenerateNewParticle(ParticleEffectComponent* effect, ParticleGroup& group, float32 currLoopTime, const Matrix4& worldTransform);
    void UpdateRegularParticleData(ParticleEffectComponent* effect, Particle* particle, const ParticleGroup& group, float32 overLife, int32 simplifiedForcesCount, Vector<Vector3>& currSimplifiedForceValues, float32 dt, AABBox3& bbox, const Vector<ParticleForce*>& effectAlignForces, uint32 effectAlignForcesCount, const Vector<ParticleForce*>& worldAlignForces, uint32 worldAlignForcesCount, const Matrix4& world, const Matrix4& invWorld, float32 layerOverLife);

    void PrepareEmitterParameters(Particle* particle, ParticleGroup& group, const Matrix4& worldTransform);
    void AddParticleToBBox(const Vector3& position, float radius, AABBox3& bbox);

    void RunEmitter(ParticleEffectComponent* effect, ParticleEmitter* emitter, const Vector3& spawnPosition, int32 positionSource = 0);

private:
    void ApplyGlobalForces(Particle* particle, float32 dt, float32 overLife, float32 layerOverLife, Vector3 prevParticlePosition);
    void UpdateStripe(Particle* particle, ParticleEffectData& effectData, ParticleGroup& group, float32 dt, AABBox3& bbox, const Vector<Vector3>& currForceValues, int32 forcesCount, bool isActive);
    void SimulateEffect(ParticleEffectComponent* effect);
    void FillEmitterRadiuses(const ParticleGroup& group, float32& radius, float32& innerRadius);

    Map<String, float32> globalExternalValues;
    Vector<ParticleEffectComponent*> activeComponents;

    struct EffectGlobalForcesData
    {
        Vector<ParticleForce*> worldAlignForces;
        Vector<ParticleForce*> effectAlignForces;
    };
    void RemoveForcesFromGlobal(ParticleEffectComponent* effect);
    void ExtractGlobalForces(ParticleEffectComponent* effect);

private: //materials stuff
    NMaterial* particleBaseMaterial;
    Vector<std::pair<MaterialData, NMaterial*>> particlesMaterials;
    Map<ParticleEffectComponent*, EffectGlobalForcesData> globalForces;
    NMaterial* AcquireMaterial(const MaterialData& materialData);

    bool allowLodDegrade;
    bool is2DMode;
};

inline const Vector<std::pair<ParticleEffectSystem::MaterialData, NMaterial*>>& ParticleEffectSystem::GetMaterialInstances() const
{
    return particlesMaterials;
}

inline void ParticleEffectSystem::SetAllowLodDegrade(bool allowDegrade)
{
    allowLodDegrade = allowDegrade;
}

inline bool ParticleEffectSystem::GetAllowLodDegrade() const
{
    return allowLodDegrade;
}
};