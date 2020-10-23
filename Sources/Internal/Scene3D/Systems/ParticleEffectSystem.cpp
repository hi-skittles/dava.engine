#include "Scene3D/Systems/ParticleEffectSystem.h"

#include <limits>

#include "Math/MathConstants.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticlesRandom.h"
#include "Particles/ParticleForces.h"
#include "Particles/ParticleForce.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Time/SystemTimer.h"
#include "Utils/Random.h"
#include "Core/PerformanceSettings.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Render/Material/NMaterialNames.h"
#include "Render/RHI/rhi_ShaderSource.h"
#include "Particles/ParticleRenderObject.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Render/Renderer.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

namespace DAVA
{
namespace ParticleEffectSystemDetails
{
Matrix3 GenerateEmitterRotationMatrix(Vector3 vector, float32 power)
{
    Vector3 axis(vector.y, -vector.x, 0);
    axis.Normalize();
    float32 angle = std::acos(vector.z / power);
    Matrix3 rotation;
    rotation.CreateRotation(axis, angle);
    return rotation;
}
}

NMaterial* ParticleEffectSystem::AcquireMaterial(const MaterialData& materialData)
{
    if (materialData.texture == nullptr) //for superemitter particles eg
        return nullptr;

    for (auto& particlesMaterial : particlesMaterials)
    {
        if (particlesMaterial.first == materialData)
            return particlesMaterial.second;
    }

    NMaterial* material = new NMaterial();
    material->SetParent(particleBaseMaterial);

    if (materialData.enableFrameBlend)
        material->AddFlag(NMaterialFlagName::FLAG_FRAME_BLEND, 1);

    if ((!materialData.enableFog) || (is2DMode)) //inverse logic to suspend vertex fog inherited from global material
    {
        material->AddFlag(NMaterialFlagName::FLAG_VERTEXFOG, 0);
    }

    if (is2DMode)
        material->AddFlag(NMaterialFlagName::FLAG_FORCE_2D_MODE, 1);

    if (materialData.enableFlow && materialData.flowmap != nullptr)
    {
        material->AddFlag(NMaterialFlagName::FLAG_PARTICLES_FLOWMAP, 1);
        material->AddTexture(NMaterialTextureName::TEXTURE_FLOW, materialData.flowmap);
    }

    if (materialData.enableNoise && materialData.noise != nullptr)
    {
        material->AddFlag(NMaterialFlagName::FLAG_PARTICLES_NOISE, 1);
        material->AddTexture(NMaterialTextureName::TEXTURE_NOISE, materialData.noise);
    }

    if (materialData.useFresnelToAlpha)
    {
        material->AddFlag(NMaterialFlagName::FLAG_PARTICLES_FRESNEL_TO_ALPHA, 1);
    }

    if (materialData.enableFlowAnimation)
    {
        material->AddFlag(NMaterialFlagName::FLAG_PARTICLES_FLOWMAP_ANIMATION, 1);
    }

    if (materialData.enableAlphaRemap && materialData.alphaRemapTexture)
    {
        material->AddFlag(NMaterialFlagName::FLAG_PARTICLES_ALPHA_REMAP, 1);
        material->AddTexture(NMaterialTextureName::TEXTURE_ALPHA_REMAP, materialData.alphaRemapTexture);
    }

    if (materialData.usePerspectiveMapping)
    {
        material->AddFlag(NMaterialFlagName::FLAG_PARTICLES_PERSPECTIVE_MAPPING, 1);
    }

    if (materialData.useThreePointGradient)
    {
        material->AddFlag(NMaterialFlagName::FLAG_PARTICLES_THREE_POINT_GRADIENT, 1);
        float32 tmp[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        material->AddProperty(NMaterialParamName::PARAM_PARTICLES_GRADIENT_COLOR_FOR_WHITE, tmp, rhi::ShaderProp::TYPE_FLOAT4);
        material->AddProperty(NMaterialParamName::PARAM_PARTICLES_GRADIENT_COLOR_FOR_BLACK, tmp, rhi::ShaderProp::TYPE_FLOAT4);
        material->AddProperty(NMaterialParamName::PARAM_PARTICLES_GRADIENT_COLOR_FOR_MIDDLE, tmp, rhi::ShaderProp::TYPE_FLOAT4);
        material->AddProperty(NMaterialParamName::PARAM_PARTICLES_GRADIENT_MIDDLE_POINT, tmp, rhi::ShaderProp::TYPE_FLOAT1);
    }

    material->AddTexture(NMaterialTextureName::TEXTURE_ALBEDO, materialData.texture);
    material->AddFlag(NMaterialFlagName::FLAG_BLENDING, materialData.blending);

    particlesMaterials.push_back(std::make_pair(materialData, material));

    material->PreBuildMaterial(PASS_FORWARD);

    return material;
}

ParticleEffectSystem::ParticleEffectSystem(Scene* scene, bool _is2DMode)
    : SceneSystem(scene)
    , allowLodDegrade(false)
    , is2DMode(_is2DMode)
{
    if (scene) //for 2d particles there would be no scene
    {
        scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::START_PARTICLE_EFFECT);
        scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::STOP_PARTICLE_EFFECT);
    }

    particleBaseMaterial = new NMaterial();
    particleBaseMaterial->SetFXName(NMaterialName::PARTICLES);
}

ParticleEffectSystem::~ParticleEffectSystem()
{
    for (auto& it : particlesMaterials)
        SafeRelease(it.second);

    SafeRelease(particleBaseMaterial);
}

void ParticleEffectSystem::SetGlobalMaterial(NMaterial* material)
{
    particleBaseMaterial->SetParent(material);

    //RHI_COMPLETE pre-cache all configs for regularly used blending modes
    const static uint32 FRAME_BLEND_MASK = 1;
    const static uint32 FOG_MASK = 2;
    const static uint32 BLEND_SHIFT = 2;
    for (uint32 i = 0; i < 12; i++)
    {
        bool enableFrameBlend = (i & FRAME_BLEND_MASK) == FRAME_BLEND_MASK;
        bool enableFog = (i & FOG_MASK) == FOG_MASK;
        uint32 blending = (i >> BLEND_SHIFT) + 1;

        ScopedPtr<NMaterial> material(new NMaterial());
        material->SetParent(particleBaseMaterial);

        if (enableFrameBlend)
            material->AddFlag(NMaterialFlagName::FLAG_FRAME_BLEND, 1);
        if (!enableFog) //inverse logic to suspend vertex fog inherited from global material
            material->AddFlag(NMaterialFlagName::FLAG_VERTEXFOG, 0);
        material->AddFlag(NMaterialFlagName::FLAG_BLENDING, blending);
        material->PreCacheFX();
    }
}

void ParticleEffectSystem::PrebuildMaterials(ParticleEffectComponent* component)
{
    for (auto& emitter : component->emitterInstances)
    {
        for (auto layer : emitter->GetEmitter()->layers)
        {
            if (layer->sprite && (layer->type != ParticleLayer::TYPE_SUPEREMITTER_PARTICLES))
            {
                DAVA::Texture* flowmap = layer->flowmap.get() != nullptr ? layer->flowmap->GetTexture(0) : nullptr;
                DAVA::Texture* noise = layer->noise.get() != nullptr ? layer->noise->GetTexture(0) : nullptr;
                DAVA::Texture* alphaRemap = layer->alphaRemapSprite.get() != nullptr ? layer->alphaRemapSprite->GetTexture(0) : nullptr;
                ParticleEffectSystem::MaterialData matData = {};
                matData.texture = layer->sprite->GetTexture(0);
                matData.enableFog = layer->enableFog;
                matData.enableFrameBlend = layer->enableFrameBlend;
                matData.flowmap = flowmap;
                matData.enableFlowAnimation = layer->enableFlowAnimation;
                matData.enableFlow = layer->enableFlow;
                matData.enableNoise = layer->enableNoise;
                matData.noise = noise;
                matData.useFresnelToAlpha = layer->useFresnelToAlpha;
                matData.blending = layer->blending;
                matData.enableAlphaRemap = layer->enableAlphaRemap;
                matData.alphaRemapTexture = alphaRemap;
                matData.usePerspectiveMapping = layer->usePerspectiveMapping && layer->type == ParticleLayer::TYPE_PARTICLE_STRIPE;
                matData.useThreePointGradient = layer->useThreePointGradient;
                uintptr_t layerIdPtr = reinterpret_cast<uintptr_t>(layer);
                matData.layerId = static_cast<uint64>(layerIdPtr);

                AcquireMaterial(matData);
            }
        }
    }
}

void ParticleEffectSystem::RunEmitter(ParticleEffectComponent* effect, ParticleEmitter* emitter, const Vector3& spawnPosition, int32 positionSource)
{
    for (ParticleLayer* layer : emitter->layers)
    {
        bool isLodActive = layer->IsLodActive(effect->activeLodLevel);
        if (!isLodActive && emitter->shortEffect) //layer could never become active
            continue;

        ParticleGroup group;
        group.emitter = SafeRetain(emitter);
        group.layer = SafeRetain(layer);
        group.spawnPosition = spawnPosition;
        group.visibleLod = isLodActive;
        group.positionSource = positionSource;
        group.loopLayerStartTime = group.layer->startTime;
        group.loopDuration = group.layer->endTime;

        if (layer->sprite && (layer->type != ParticleLayer::TYPE_SUPEREMITTER_PARTICLES))
        {
            DAVA::Texture* flowmap = layer->flowmap.get() != nullptr ? layer->flowmap->GetTexture(0) : nullptr;
            DAVA::Texture* noise = layer->noise.get() != nullptr ? layer->noise->GetTexture(0) : nullptr;
            DAVA::Texture* alphaRemap = layer->alphaRemapSprite.get() != nullptr ? layer->alphaRemapSprite->GetTexture(0) : nullptr;
            ParticleEffectSystem::MaterialData matData = {};
            matData.texture = layer->sprite->GetTexture(0);
            matData.enableFog = layer->enableFog;
            matData.enableFrameBlend = layer->enableFrameBlend && layer->type != ParticleLayer::TYPE_PARTICLE_STRIPE;
            matData.flowmap = flowmap;
            matData.enableFlowAnimation = layer->enableFlowAnimation;
            matData.enableFlow = layer->enableFlow;
            matData.enableNoise = layer->enableNoise;
            matData.noise = noise;
            matData.useFresnelToAlpha = layer->useFresnelToAlpha;
            matData.blending = layer->blending;
            matData.enableAlphaRemap = layer->enableAlphaRemap;
            matData.alphaRemapTexture = alphaRemap;
            matData.usePerspectiveMapping = layer->usePerspectiveMapping && layer->type == ParticleLayer::TYPE_PARTICLE_STRIPE;
            matData.useThreePointGradient = layer->useThreePointGradient;
            uintptr_t layerIdPtr = reinterpret_cast<uintptr_t>(layer);
            matData.layerId = static_cast<uint64>(layerIdPtr);

            group.material = AcquireMaterial(matData);
        }

        effect->effectData.groups.push_back(group);
    }
}

void ParticleEffectSystem::RunEffect(ParticleEffectComponent* effect)
{
    if (QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_EFFECTS))
    {
        return;
    }

    if (effect->activeLodLevel != effect->desiredLodLevel)
    {
        UpdateActiveLod(effect);
    }

    if (effect->effectData.groups.empty()) // clean position sources
    {
        effect->effectData.infoSources.resize(1);
    }

    for (const auto& instance : effect->emitterInstances)
    {
        RunEmitter(effect, instance->GetEmitter(), instance->GetSpawnPosition());
    }

    effect->state = ParticleEffectComponent::STATE_PLAYING;
    effect->time = 0;

    if (effect->GetStartFromTime() > EPSILON)
    {
        SimulateEffect(effect);
    }
}

void ParticleEffectSystem::AddToActive(ParticleEffectComponent* effect)
{
    if (QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_EFFECTS))
    {
        return;
    }

    if (effect->state == ParticleEffectComponent::STATE_STOPPED)
    {
        //add to active effects and to render
        activeComponents.push_back(effect);
        for (Map<String, float32>::iterator it = globalExternalValues.begin(), e = globalExternalValues.end(); it != e; ++it)
            effect->SetExtertnalValue((*it).first, (*it).second);
        Scene* scene = GetScene();
        if (scene)
        {
            Matrix4* worldTransformPointer = effect->GetEntity()->GetComponent<TransformComponent>()->GetWorldMatrixPtr();
            effect->effectRenderObject->SetWorldMatrixPtr(worldTransformPointer);
            Vector3 pos = worldTransformPointer->GetTranslationVector();
            effect->effectRenderObject->SetAABBox(AABBox3(pos, pos));
            scene->GetRenderSystem()->RenderPermanent(effect->effectRenderObject);
        }
    }
}

void ParticleEffectSystem::RemoveFromActive(ParticleEffectComponent* effect)
{
    if (QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_EFFECTS))
    {
        return;
    }

    Vector<ParticleEffectComponent*>::iterator it = std::find(activeComponents.begin(), activeComponents.end(), effect);
    DVASSERT(it != activeComponents.end());
    activeComponents.erase(it);
    effect->state = ParticleEffectComponent::STATE_STOPPED;
    Scene* scene = GetScene();
    if (scene)
        scene->GetRenderSystem()->RemoveFromRender(effect->effectRenderObject);
}

void ParticleEffectSystem::AddEntity(Entity* entity)
{
    ParticleEffectComponent* effect = entity->GetComponent<ParticleEffectComponent>();
    PrebuildMaterials(effect);
}

void ParticleEffectSystem::AddComponent(Entity* entity, Component* component)
{
    ParticleEffectComponent* effect = static_cast<ParticleEffectComponent*>(component);
    PrebuildMaterials(effect);
}

void ParticleEffectSystem::RemoveEntity(Entity* entity)
{
    ParticleEffectComponent* effect = entity->GetComponent<ParticleEffectComponent>();
    if (effect && effect->state != ParticleEffectComponent::STATE_STOPPED)
        RemoveFromActive(effect);
}

void ParticleEffectSystem::RemoveComponent(Entity* entity, Component* component)
{
    ParticleEffectComponent* effect = static_cast<ParticleEffectComponent*>(component);
    if (effect && effect->state != ParticleEffectComponent::STATE_STOPPED)
        RemoveFromActive(effect);
}

void ParticleEffectSystem::PrepareForRemove()
{
    if (QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_EFFECTS) == false)
    {
        for (ParticleEffectComponent* component : activeComponents)
        {
            component->state = ParticleEffectComponent::STATE_STOPPED;
            Scene* scene = GetScene();
            if (scene)
                scene->GetRenderSystem()->RemoveFromRender(component->effectRenderObject);
        }
    }
    activeComponents.clear();
    globalExternalValues.clear();
}

void ParticleEffectSystem::ImmediateEvent(Component* component, uint32 event)
{
    DVASSERT(component->GetType()->Is<ParticleEffectComponent>());

    ParticleEffectComponent* effect = static_cast<ParticleEffectComponent*>(component);
    if (event == EventSystem::START_PARTICLE_EFFECT)
    {
        if (effect->state == ParticleEffectComponent::STATE_STOPPED)
            AddToActive(effect);
        effect->state = ParticleEffectComponent::STATE_STARTING;
        effect->currRepeatsCont = 0;

        ExtractGlobalForces(effect);
    }
    else if (event == EventSystem::STOP_PARTICLE_EFFECT)
    {
        RemoveFromActive(effect);

        RemoveForcesFromGlobal(effect);
    }
}

void ParticleEffectSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_PARTICLE_SYSTEM);

    if (timeElapsed == 0.f)
    {
        timeElapsed = 0.000001f;
    }

    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::UPDATE_PARTICLE_EMMITERS))
        return;
    /*shortEffectTime*/
    float32 currFps = 1.0f / timeElapsed;
    PerformanceSettings* perfSettings = GetEngineContext()->performanceSettings;
    float32 currPSValue = (currFps - perfSettings->GetPsPerformanceMinFPS()) / (perfSettings->GetPsPerformanceMaxFPS() - perfSettings->GetPsPerformanceMinFPS());
    currPSValue = Clamp(currPSValue, 0.0f, 1.0f);
    float32 speedMult = 1.0f + (perfSettings->GetPsPerformanceSpeedMult() - 1.0f) * (1 - currPSValue);
    float32 shortEffectTime = timeElapsed * speedMult;

    size_t componentsCount = activeComponents.size();
    for (size_t i = 0; i < componentsCount; i++)
    {
        ParticleEffectComponent* effect = activeComponents[i];
        if (effect->activeLodLevel != effect->desiredLodLevel)
            UpdateActiveLod(effect);
        if (effect->state == ParticleEffectComponent::STATE_STARTING)
        {
            RunEffect(effect);
        }

        if (effect->isPaused)
            continue;
        UpdateEffect(effect, timeElapsed * effect->playbackSpeed, shortEffectTime * effect->playbackSpeed);

        bool effectEnded = effect->stopWhenEmpty ? effect->effectData.groups.empty() : (effect->time > effect->effectDuration);
        if (effectEnded)
        {
            effect->currRepeatsCont++;
            if (((effect->repeatsCount == 0) || (effect->currRepeatsCont < effect->repeatsCount)) && (effect->state != ParticleEffectComponent::STATE_STOPPING)) //0 means infinite repeats
            {
                if (effect->clearOnRestart)
                    effect->ClearCurrentGroups();
                RunEffect(effect);
            }
            else
            {
                effect->state = ParticleEffectComponent::STATE_STOPPING;
                effect->SetGroupsFinishing();
            }
        }
        /*finish restart criteria*/
        if ((effect->state == ParticleEffectComponent::STATE_STOPPING) && effect->effectData.groups.empty())
        {
            effect->effectData.infoSources.resize(1);
            RemoveFromActive(effect);
            componentsCount--;
            i--;
            effect->state = ParticleEffectComponent::STATE_STOPPED;
            if (!effect->playbackComplete.IsEmpty())
                effect->playbackComplete(effect->GetEntity(), 0);
        }
        else
        {
            Scene* scene = GetScene();
            if (scene)
                scene->GetRenderSystem()->MarkForUpdate(effect->effectRenderObject);
        }
    }
}

void ParticleEffectSystem::UpdateActiveLod(ParticleEffectComponent* effect)
{
    DVASSERT(effect->activeLodLevel != effect->desiredLodLevel);
    effect->activeLodLevel = effect->desiredLodLevel;
    for (List<ParticleGroup>::iterator it = effect->effectData.groups.begin(), e = effect->effectData.groups.end(); it != e; ++it)
    {
        ParticleGroup& group = *it;
        if (!group.emitter->shortEffect)
            group.visibleLod = group.layer->IsLodActive(effect->activeLodLevel);
    }

    if (allowLodDegrade && (effect->activeLodLevel == 0)) //degrade existing groups if needed
    {
        for (List<ParticleGroup>::iterator it = effect->effectData.groups.begin(), e = effect->effectData.groups.end(); it != e; ++it)
        {
            ParticleGroup& group = *it;
            if (group.layer->degradeStrategy == ParticleLayer::DEGRADE_REMOVE)
            {
                Particle* current = group.head;
                while (current)
                {
                    Particle* next = current->next;
                    delete current;
                    current = next;
                }
                group.head = nullptr;
            }
            else if (group.layer->degradeStrategy == ParticleLayer::DEGRADE_CUT_PARTICLES)
            {
                Particle* current = group.head;
                Particle* prev = nullptr;
                int32 i = 0;
                while (current)
                {
                    Particle* next = current->next;
                    if (i % 2) //cut every second particle
                    {
                        delete current;
                        group.activeParticleCount--;
                        if (prev)
                            prev->next = next;
                        else
                            group.head = next;
                    }
                    prev = current;
                    current = next;
                    i++;
                }
            }
        }
    }
}

void ParticleEffectSystem::UpdateEffect(ParticleEffectComponent* effect, float32 deltaTime, float32 shortEffectTime)
{
    effect->time += deltaTime;
    const Matrix4* worldTransformPtr;
    if (GetScene())
    {
        TransformComponent* tr = GetTransformComponent(effect->GetEntity());
        DVASSERT(tr);
        worldTransformPtr = tr->GetWorldMatrixPtr();
    }
    else
        worldTransformPtr = effect->effectRenderObject->GetWorldMatrixPtr();

    effect->effectData.infoSources[0].position = worldTransformPtr->GetTranslationVector();

    AABBox3 bbox;
    List<ParticleGroup>::iterator it = effect->effectData.groups.begin();
    Random* random = GetEngineContext()->random;
    bool isInverseCalculated = false;
    while (it != effect->effectData.groups.end())
    {
        ParticleGroup& group = *it;
        group.activeParticleCount = 0;
        float32 dt = group.emitter->shortEffect ? shortEffectTime : deltaTime;
        group.time += dt;
        float32 groupEndTime = group.layer->isLooped ? group.layer->loopEndTime : group.layer->endTime;
        float32 currLoopTime = group.time - group.loopStartTime;
        float32 currLoopTimeNormalized = currLoopTime / (group.layer->endTime - group.layer->startTime);
        if (group.time > groupEndTime)
            group.finishingGroup = true;

        if ((!group.finishingGroup) && (group.layer->isLooped) && (currLoopTime > group.loopDuration)) //restart loop
        {
            group.loopStartTime = group.time;
            group.loopLayerStartTime = group.layer->deltaTime + group.layer->deltaVariation * static_cast<float32>(random->RandFloat());
            group.loopDuration = group.loopLayerStartTime + (group.layer->endTime - group.layer->startTime) + group.layer->loopVariation * static_cast<float32>(random->RandFloat());
            currLoopTime = 0;
        }

        //prepare forces as they will now actually change in time even for already generated particles
        static Vector<Vector3> currSimplifiedForceValues;
        int32 simplifiedForcesCount;

        static Vector<ParticleForce*> effectAlignCurrForces;
        static Vector<ParticleForce*> worldAlignCurrForces;
        uint32 forcesCountWorldAlign = 0;
        uint32 effectAlignForcesCount = 0;

        static Matrix4 invWorld;
        if (group.head)
        {
            simplifiedForcesCount = static_cast<int32>(group.layer->GetSimplifiedParticleForces().size());
            if (simplifiedForcesCount)
            {
                currSimplifiedForceValues.resize(simplifiedForcesCount);
                for (int32 i = 0; i < simplifiedForcesCount; ++i)
                {
                    if (group.layer->GetSimplifiedParticleForces()[i]->force)
                        currSimplifiedForceValues[i] = group.layer->GetSimplifiedParticleForces()[i]->force->GetValue(currLoopTime);
                    else
                        currSimplifiedForceValues[i] = Vector3(0, 0, 0);
                }
            }

            uint32 allForcesCount = static_cast<uint32>(group.layer->GetParticleForces().size());
            if (allForcesCount > 0)
            {
                effectAlignCurrForces.resize(allForcesCount);
                worldAlignCurrForces.resize(allForcesCount);
                for (uint32 i = 0; i < allForcesCount; ++i)
                {
                    DAVA::ParticleForce* currForce = group.layer->GetParticleForces()[i];
                    if (currForce->isGlobal)
                        continue;

                    if (currForce->worldAlign)
                    {
                        currForce->worldPosition = currForce->position + worldTransformPtr->GetTranslationVector(); // Ignore emitter rotation.
                        worldAlignCurrForces[forcesCountWorldAlign] = currForce;
                        ++forcesCountWorldAlign;
                    }
                    else
                    {
                        effectAlignCurrForces[effectAlignForcesCount] = currForce;
                        ++effectAlignForcesCount;
                        if (!isInverseCalculated)
                        {
                            invWorld = GetInverseWithRemovedScale(*worldTransformPtr);
                            isInverseCalculated = true;
                        }
                    }
                }
            }
        }

        Particle* current = group.head;
        Particle* prev = nullptr;

        while (current)
        {
            current->life += dt;
            if (current->life >= current->lifeTime)
            {
                Particle* next = current->next;
                if (prev == nullptr)
                    group.head = next;
                else
                    prev->next = next;
                delete current;
                current = next;
                continue;
            }
            group.activeParticleCount++;

            float32 overLifeTime = current->life / current->lifeTime;

            if (group.layer->type != ParticleLayer::TYPE_PARTICLE_STRIPE)
            {
                UpdateRegularParticleData(effect, current, group, overLifeTime, simplifiedForcesCount, currSimplifiedForceValues, dt, bbox, effectAlignCurrForces, effectAlignForcesCount, worldAlignCurrForces, forcesCountWorldAlign, *worldTransformPtr, invWorld, currLoopTimeNormalized);
            }

            if (group.layer->type == ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
            {
                effect->effectData.infoSources[current->positionTarget].position = current->position;
                effect->effectData.infoSources[current->positionTarget].size = current->currSize;
            }

            if (group.layer->enableNoise && group.layer->noise.get() != nullptr)
            {
                if (group.layer->noiseScaleOverLife != nullptr)
                    current->currNoiseScale = current->baseNoiseScale * group.layer->noiseScaleOverLife->GetValue(overLifeTime);

                DAVA::float32 overLifeScale = 1.0f;
                if (group.layer->noiseUScrollSpeedOverLife != nullptr)
                {
                    overLifeScale = group.layer->noiseUScrollSpeedOverLife->GetValue(overLifeTime);
                }
                current->currNoiseUOffset += current->baseNoiseUScrollSpeed * overLifeScale * deltaTime;

                overLifeScale = 1.0f;
                if (group.layer->noiseVScrollSpeedOverLife != nullptr)
                {
                    overLifeScale = group.layer->noiseVScrollSpeedOverLife->GetValue(overLifeTime);
                }
                current->currNoiseVOffset += current->baseNoiseVScrollSpeed * overLifeScale * deltaTime;
            }

            if (group.layer->enableAlphaRemap && group.layer->alphaRemapSprite.get() != nullptr && group.layer->alphaRemapOverLife != nullptr)
            {
                float32 lookup = overLifeTime * group.layer->alphaRemapLoopCount;
                float32 intPart;
                current->alphaRemap = group.layer->alphaRemapOverLife->GetValue(modff(lookup, &intPart));
            }

            if (group.layer->type == ParticleLayer::TYPE_PARTICLE_STRIPE)
                UpdateStripe(current, effect->effectData, group, deltaTime, bbox, currSimplifiedForceValues, simplifiedForcesCount, group.layer->IsLodActive(effect->activeLodLevel));

            prev = current;
            current = current->next;
        }
        bool allowParticleGeneration = !group.finishingGroup;
        allowParticleGeneration &= (currLoopTime > group.loopLayerStartTime);
        allowParticleGeneration &= group.visibleLod;
        if (allowParticleGeneration)
        {
            if (group.layer->type == ParticleLayer::TYPE_SINGLE_PARTICLE || group.layer->type == ParticleLayer::TYPE_PARTICLE_STRIPE)
            {
                if (!group.head)
                {
                    current = GenerateNewParticle(effect, group, currLoopTime, *worldTransformPtr);
                    if (group.layer->GetInheritPosition())
                        AddParticleToBBox(current->position + effect->effectData.infoSources[group.positionSource].position, current->currRadius, bbox);
                    else
                        AddParticleToBBox(current->position, current->currRadius, bbox);
                }
            }
            else
            {
                float32 newParticles = 0.0f;

                if (group.layer->number)
                    newParticles = group.layer->number->GetValue(currLoopTime);
                if (group.layer->numberVariation)
                    newParticles += group.layer->numberVariation->GetValue(currLoopTime) * static_cast<float32>(random->RandFloat());
                newParticles *= dt;
                group.particlesToGenerate += newParticles;

                while (group.particlesToGenerate >= 1.0f)
                {
                    group.particlesToGenerate -= 1.0f;
                    current = GenerateNewParticle(effect, group, currLoopTime, *worldTransformPtr);
                    if (group.layer->GetInheritPosition())
                        AddParticleToBBox(current->position + effect->effectData.infoSources[group.positionSource].position, current->currRadius, bbox);
                    else
                        AddParticleToBBox(current->position, current->currRadius, bbox);
                }
            }
        }

        if (group.finishingGroup && (group.head == nullptr))
        {
            DAVA::SafeRelease(group.emitter);
            DAVA::SafeRelease(group.layer);
            it = effect->effectData.groups.erase(it);
        }
        else
        {
            ++it;
        }
    }
    if (bbox.IsEmpty())
    {
        Vector3 pos = worldTransformPtr->GetTranslationVector();
        bbox = AABBox3(pos, pos);
    }
    effect->effectRenderObject->SetAABBox(bbox);
}

void ParticleEffectSystem::UpdateStripe(Particle* particle, ParticleEffectData& effectData, ParticleGroup& group, float32 dt, AABBox3& bbox, const Vector<Vector3>& currForceValues, int32 forcesCount, bool isActive)
{
    ParticleLayer* layer = group.layer;
    StripeData& data = group.stripe;
    Vector3 prevBasePosition = data.baseNode.position;
    data.baseNode.position = particle->position;
    data.isActive = isActive;

    if (layer->GetInheritPosition())
    {
        data.inheritPositionOffset = effectData.infoSources[group.positionSource].position;
    }

    if (layer->GetInheritPositionForStripeBase())
    {
        data.baseNode.position = effectData.infoSources[group.positionSource].position;
    }

    data.baseNode.speed = particle->speed;

    bool shouldInsert = data.stripeNodes.empty() || (data.baseNode.position - data.stripeNodes.front().position).SquareLength() > layer->stripeVertexSpawnStep * layer->stripeVertexSpawnStep;

    float32 radius = layer->stripeStartSize * layer->CalculateMaxStripeSizeOverLife();

    if (shouldInsert)
    {
        data.stripeNodes.emplace_front(0.0f, data.baseNode.position, data.baseNode.speed, 0.0f, 0.0f);
        data.prevBaseLen = 0.0f;
    }

    auto nodeIter = data.stripeNodes.begin();
    DAVA::StripeNode* prevNode = &data.baseNode;

    float32 firstDeltaLen = 0.0f;
    Vector3 prevFirstNodePosition = data.baseNode.position;
    if (data.stripeNodes.size() > 0)
        prevFirstNodePosition = nodeIter->position;

    while (nodeIter != data.stripeNodes.end())
    {
        nodeIter->lifeime += dt;
        float32 overLife = nodeIter->lifeime / layer->stripeLifetime;

        float32 currVelocityOverLife = 1.0f;
        if (layer->velocityOverLife)
            currVelocityOverLife = layer->velocityOverLife->GetValue(overLife);
        nodeIter->position += nodeIter->speed * (currVelocityOverLife * dt);

        if (nodeIter == data.stripeNodes.begin())
            firstDeltaLen = (prevFirstNodePosition - nodeIter->position).Length();

        if (forcesCount > 0)
        {
            Vector3 acceleration;
            for (int32 i = 0; i < forcesCount; ++i)
            {
                acceleration += (layer->GetSimplifiedParticleForces()[i]->forceOverLife) ? (currForceValues[i] * layer->GetSimplifiedParticleForces()[i]->forceOverLife->GetValue(overLife)) : currForceValues[i];
            }
            nodeIter->speed += acceleration * dt;
        }
        if (layer->GetInheritPosition())
            AddParticleToBBox(nodeIter->position + effectData.infoSources[group.positionSource].position, radius, bbox);
        else
            AddParticleToBBox(nodeIter->position, radius, bbox);

        nodeIter->distanceFromPrevNode = (prevNode->position - nodeIter->position).Length();
        nodeIter->distanceFromBase = prevNode->distanceFromBase + nodeIter->distanceFromPrevNode;

        prevNode = &(*nodeIter);
        if (nodeIter->lifeime >= layer->stripeLifetime)
            data.stripeNodes.erase(nodeIter++);
        else
            ++nodeIter;
    }

    if (layer->GetInheritPositionForStripeBase() && data.stripeNodes.size() > 0)
    {
        float32 previousLength = data.prevBaseLen;
        float32 currentLength = (data.baseNode.position - data.stripeNodes.front().position).Length();
        float32 deltaLength = previousLength - currentLength + firstDeltaLen;
        data.prevBaseLen = currentLength;

        if (!shouldInsert)
            data.uvOffset += deltaLength;
        else
        {
            float32 delta = (data.baseNode.position - prevBasePosition).Length();
            if (particle->speed.DotProduct(data.baseNode.position - prevBasePosition) <= 0)
            {
                data.uvOffset -= delta;
            }
            else
            {
                data.uvOffset -= delta - firstDeltaLen; // Look at 7abf5621f27 commit (and before) if results will be unsatisfying at some point.
            }
        }
    }
}

void ParticleEffectSystem::AddParticleToBBox(const Vector3& position, float radius, AABBox3& bbox)
{
    Vector3 sz = Vector3(radius, radius, radius);
    bbox.AddPoint(position - sz);
    bbox.AddPoint(position + sz);
}

Particle* ParticleEffectSystem::GenerateNewParticle(ParticleEffectComponent* effect, ParticleGroup& group, float32 currLoopTime, const Matrix4& worldTransform)
{
    Particle* particle = new Particle();
    particle->life = 0.0f;

    particle->color = Color();
    if (group.layer->colorRandom)
    {
        particle->color = group.layer->colorRandom->GetValue(static_cast<float32>(GetEngineContext()->random->RandFloat()));
    }
    if (group.emitter->colorOverLife)
    {
        particle->color *= group.emitter->colorOverLife->GetValue(group.time);
    }

    particle->lifeTime = 0.0f;
    if (group.layer->life)
        particle->lifeTime += group.layer->life->GetValue(currLoopTime);
    if (group.layer->lifeVariation)
        particle->lifeTime += (group.layer->lifeVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));

    // Flow.
    particle->baseFlowSpeed = 0.0f;
    if (group.layer->flowSpeed)
        particle->baseFlowSpeed += group.layer->flowSpeed->GetValue(currLoopTime);
    if (group.layer->flowSpeedVariation)
        particle->baseFlowSpeed += (group.layer->flowSpeedVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    particle->currFlowSpeed = particle->baseFlowSpeed;

    particle->baseFlowOffset = 0.0f;
    if (group.layer->flowOffset)
        particle->baseFlowOffset += group.layer->flowOffset->GetValue(currLoopTime);
    if (group.layer->flowOffsetVariation)
        particle->baseFlowOffset += (group.layer->flowOffsetVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    particle->currFlowOffset = particle->baseFlowOffset;

    // Noise.
    particle->baseNoiseScale = 0.0f;
    if (group.layer->noiseScale)
        particle->baseNoiseScale += group.layer->noiseScale->GetValue(currLoopTime);
    if (group.layer->noiseScaleVariation)
        particle->baseNoiseScale += (group.layer->noiseScaleVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    particle->currNoiseScale = particle->baseNoiseScale;

    particle->baseNoiseUScrollSpeed = 0.0f;
    if (group.layer->noiseUScrollSpeed)
        particle->baseNoiseUScrollSpeed += group.layer->noiseUScrollSpeed->GetValue(currLoopTime);
    if (group.layer->noiseUScrollSpeedVariation)
        particle->baseNoiseUScrollSpeed += (group.layer->noiseUScrollSpeedVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    particle->currNoiseUOffset = particle->baseNoiseUScrollSpeed;

    particle->baseNoiseVScrollSpeed = 0.0f;
    if (group.layer->noiseVScrollSpeed)
        particle->baseNoiseVScrollSpeed += group.layer->noiseVScrollSpeed->GetValue(currLoopTime);
    if (group.layer->noiseVScrollSpeedVariation)
        particle->baseNoiseVScrollSpeed += (group.layer->noiseVScrollSpeedVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    particle->currNoiseVOffset = particle->baseNoiseVScrollSpeed;

    // size
    particle->baseSize = Vector2(1.0f, 1.0f);
    if (group.layer->size)
        particle->baseSize = group.layer->size->GetValue(currLoopTime);
    if (group.layer->sizeVariation)
        particle->baseSize += (group.layer->sizeVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    particle->baseSize *= effect->effectData.infoSources[group.positionSource].size;

    particle->currSize = particle->baseSize;
    if (group.layer->sizeOverLifeXY)
        particle->currSize *= group.layer->sizeOverLifeXY->GetValue(0);
    Vector2 pivotSize = particle->currSize * group.layer->layerPivotSizeOffsets;
    particle->currRadius = pivotSize.Length();

    particle->angle = 0.0f;
    particle->spin = 0.0f;
    if (group.layer->angle)
        particle->angle = DegToRad(group.layer->angle->GetValue(currLoopTime));
    if (group.layer->angleVariation)
        particle->angle += DegToRad(group.layer->angleVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    if (group.layer->spin)
        particle->spin = DegToRad(group.layer->spin->GetValue(currLoopTime));
    if (group.layer->spinVariation)
        particle->spin += DegToRad(group.layer->spinVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    if (group.layer->randomSpinDirection)
    {
        int32 dir = Rand() & 1;
        particle->spin *= (dir)*2 - 1;
    }
    particle->frame = 0;
    particle->animTime = 0;
    if (group.layer->randomFrameOnStart && group.layer->sprite)
    {
        particle->frame = static_cast<int32>(static_cast<float32>(GetEngineContext()->random->RandFloat()) * static_cast<float32>(group.layer->sprite->GetFrameCount()));
    }

    PrepareEmitterParameters(particle, group, worldTransform);

    float32 vel = 0.0f;
    if (group.layer->velocity)
        vel += group.layer->velocity->GetValue(currLoopTime);
    if (group.layer->velocityVariation)
        vel += (group.layer->velocityVariation->GetValue(currLoopTime) * static_cast<float32>(GetEngineContext()->random->RandFloat()));
    particle->speed *= vel;

    if (!group.layer->GetInheritPosition()) //just generate at correct position
    {
        particle->position += effect->effectData.infoSources[group.positionSource].position;
    }

    particle->next = group.head;
    group.head = particle;
    group.activeParticleCount++;
    if (group.layer->type == ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
    {
        ParentInfo info;
        info.position = particle->position;
        info.size = particle->currSize;
        effect->effectData.infoSources.push_back(info);
        particle->positionTarget = static_cast<int32>(effect->effectData.infoSources.size() - 1);
        ParticleEmitter* innerEmitter = group.layer->innerEmitter->GetEmitter();
        if (innerEmitter)
            RunEmitter(effect, innerEmitter, Vector3(0, 0, 0), particle->positionTarget);
    }

    group.particlesGenerated++;
    return particle;
}

void ParticleEffectSystem::UpdateRegularParticleData(ParticleEffectComponent* effect, Particle* particle, const ParticleGroup& group, float32 overLife, int32 simplifiedForcesCount, Vector<Vector3>& currSimplifiedForceValues, float32 dt, AABBox3& bbox, const Vector<ParticleForce*>& effectAlignForces, uint32 effectAlignForcesCount, const Vector<ParticleForce*>& worldAlignForces, uint32 worldAlignForcesCount, const Matrix4& world, const Matrix4& invWorld, float32 layerOverLife)
{
    float32 currVelocityOverLife = 1.0f;
    if (group.layer->velocityOverLife)
        currVelocityOverLife = group.layer->velocityOverLife->GetValue(overLife);
    Vector3 prevParticlePosition = particle->position;
    particle->position += particle->speed * (currVelocityOverLife * dt);

    float32 currSpinOverLife = 1.0f;
    if (group.layer->spinOverLife)
        currSpinOverLife = group.layer->spinOverLife->GetValue(overLife);
    particle->angle += particle->spin * currSpinOverLife * dt;

    Vector3 acceleration(0.0f, 0.0f, 0.0f);
    for (int32 i = 0; i < simplifiedForcesCount; ++i)
    {
        acceleration += (group.layer->GetSimplifiedParticleForces()[i]->forceOverLife) ? (currSimplifiedForceValues[i] * group.layer->GetSimplifiedParticleForces()[i]->forceOverLife->GetValue(overLife)) : currSimplifiedForceValues[i];
    }

    for (uint32 i = 0; i < worldAlignForcesCount; ++i)
        ParticleForces::ApplyForce(worldAlignForces[i], particle->speed, particle->position, dt, overLife, layerOverLife, Vector3(0.0f, 0.0f, -1.0f), particle, prevParticlePosition, worldAlignForces[i]->worldPosition);

    if (effectAlignForcesCount > 0)
    {
        Vector3 effectSpacePosition;
        Vector3 prevEffectSpacePosition;
        Vector3 effectSpaceSpeed;
        effectSpacePosition = particle->position * invWorld;
        effectSpaceSpeed = particle->speed * Matrix3(invWorld);
        if (group.layer->GetPlaneCollisiontForcesCount() > 0)
            prevEffectSpacePosition = prevParticlePosition * invWorld;

        for (uint32 i = 0; i < effectAlignForcesCount; ++i)
            ParticleForces::ApplyForce(effectAlignForces[i], effectSpaceSpeed, effectSpacePosition, dt, overLife, layerOverLife, -Vector3(invWorld._20, invWorld._21, invWorld._22), particle, prevEffectSpacePosition, effectAlignForces[i]->position);

        particle->speed = effectSpaceSpeed * Matrix3(world);
        if (group.layer->GetAlterPositionForcesCount() > 0)
            particle->position = effectSpacePosition * world;
    }

    if (group.layer->applyGlobalForces)
        ApplyGlobalForces(particle, dt, overLife, layerOverLife, prevParticlePosition);

    particle->speed += acceleration * dt;

    if (group.layer->sizeOverLifeXY)
    {
        particle->currSize = particle->baseSize * group.layer->sizeOverLifeXY->GetValue(overLife);
        Vector2 pivotSize = particle->currSize * group.layer->layerPivotSizeOffsets;
        particle->currRadius = pivotSize.Length();
    }
    if (group.layer->GetInheritPosition())
        AddParticleToBBox(particle->position + effect->effectData.infoSources[group.positionSource].position, particle->currRadius, bbox);
    else
        AddParticleToBBox(particle->position, particle->currRadius, bbox);

    if (group.layer->frameOverLifeEnabled && group.layer->sprite)
    {
        float32 animDelta = group.layer->frameOverLifeFPS;
        if (group.layer->animSpeedOverLife)
            animDelta *= group.layer->animSpeedOverLife->GetValue(overLife);
        particle->animTime += animDelta * dt;

        while (particle->animTime > 1.0f)
        {
            particle->frame++;
            particle->animTime -= 1.0f;
            if (particle->frame >= group.layer->sprite->GetFrameCount())
            {
                if (group.layer->loopSpriteAnimation)
                    particle->frame = 0;
                else
                    particle->frame = group.layer->sprite->GetFrameCount() - 1;
            }
        }
    }
}

void ParticleEffectSystem::ApplyGlobalForces(Particle* particle, float32 dt, float32 overLife, float32 layerOverLife, Vector3 prevParticlePosition)
{
    for (auto& forcePair : globalForces)
    {
        ParticleEffectComponent* effect = forcePair.first;
        TransformComponent* tr = GetTransformComponent(effect->GetEntity());
        Matrix4* worldTransformPtr = tr->GetWorldMatrixPtr();

        for (ParticleForce* force : forcePair.second.worldAlignForces)
        {
            Vector3 forceWorldPosition = worldTransformPtr->GetTranslationVector() + force->position;
            if (force->isInfinityRange || (forceWorldPosition - particle->position).SquareLength() < force->GetSquaredRadius())
                ParticleForces::ApplyForce(force, particle->speed, particle->position, dt, overLife, layerOverLife, Vector3(0.0f, 0.0f, -1.0f), particle, prevParticlePosition, forceWorldPosition);
        }

        if (!forcePair.second.effectAlignForces.empty())
        {
            bool inForceBoundingSphere = false;
            for (ParticleForce* force : forcePair.second.effectAlignForces) // Check if particle position in at least one global effect aligned force's bounding sphere.
            {
                if (force->isInfinityRange)
                {
                    inForceBoundingSphere = true;
                    break;
                }
                Vector3 forceWorldPosition = worldTransformPtr->GetTranslationVector() + force->position; // Do not rotate global forces if force position is not zero.
                float32 sqrDist = (forceWorldPosition - particle->position).SquareLength();
                if (sqrDist < force->GetSquaredRadius())
                {
                    inForceBoundingSphere = true;
                    break;
                }
            }
            if (!inForceBoundingSphere)
                continue;

            Matrix4 invWorld = GetInverseWithRemovedScale(*worldTransformPtr);

            Vector3 effectSpacePosition = particle->position * invWorld;
            Vector3 prevEffectSpacePosition = prevParticlePosition * invWorld;
            Vector3 effectSpaceSpeed = particle->speed * Matrix3(invWorld);
            bool transformPosition = false;
            for (ParticleForce* force : forcePair.second.effectAlignForces)
            {
                if (force->CanAlterPosition())
                    transformPosition = true;
                ParticleForces::ApplyForce(force, effectSpaceSpeed, effectSpacePosition, dt, overLife, layerOverLife, -Vector3(invWorld._20, invWorld._21, invWorld._22), particle, prevEffectSpacePosition, force->position);
            }
            particle->speed = effectSpaceSpeed * Matrix3(*worldTransformPtr);
            if (transformPosition)
                particle->position = effectSpacePosition * (*worldTransformPtr);
        }
    }
}

void ParticleEffectSystem::PrepareEmitterParameters(Particle* particle, ParticleGroup& group, const Matrix4& worldTransform)
{
    //calculate position new particle position in emitter space (for point leave it V3(0,0,0))
    uintptr_t uptr = reinterpret_cast<uintptr_t>(&group);
    uint32 offset = static_cast<uint32>(uptr);
    uint32 ind = group.particlesGenerated + offset;

    bool isCircleEmitter = group.emitter->emitterType == ParticleEmitter::EMITTER_ONCIRCLE_VOLUME || group.emitter->emitterType == ParticleEmitter::EMITTER_ONCIRCLE_EDGES;
    bool isSphereEmitter = group.emitter->emitterType == ParticleEmitter::EMITTER_SPHERE;

    // In VanDerCorput random we use different bases to avoid diagonal patterns.
    if (group.emitter->emitterType == ParticleEmitter::EMITTER_RECT)
    {
        if (group.emitter->size)
        {
            Vector3 currSize = group.emitter->size->GetValue(group.time);
            particle->position = Vector3(currSize.x * (ParticlesRandom::VanDerCorputRnd(ind, 3) - 0.5f), currSize.y * (ParticlesRandom::VanDerCorputRnd(ind, 2) - 0.5f), currSize.z * (ParticlesRandom::VanDerCorputRnd(ind, 5) - 0.5f));
        }
    }
    else if (isCircleEmitter)
    {
        float32 curRadius = 1.0f;
        float32 innerRadius = 0.0f;
        FillEmitterRadiuses(group, curRadius, innerRadius);

        float32 angleBase = 0;
        float32 angleVariation = PI_2;
        if (group.emitter->emissionAngle)
            angleBase = DegToRad(group.emitter->emissionAngle->GetValue(group.time));
        if (group.emitter->emissionAngleVariation)
            angleVariation = DegToRad(group.emitter->emissionAngleVariation->GetValue(group.time));

        float32 curAngle = angleBase + angleVariation * ParticlesRandom::VanDerCorputRnd(ind, 3);
        if (group.emitter->emitterType == ParticleEmitter::EMITTER_ONCIRCLE_VOLUME)
        {
            float32 rndRadiusNorm = std::sqrt(static_cast<float32>(GetEngineContext()->random->RandFloat())); // Better distribution on circle.
            curRadius = Lerp(innerRadius, curRadius, rndRadiusNorm);
        }
        float32 sinAngle = 0.0f;
        float32 cosAngle = 0.0f;
        SinCosFast(curAngle, sinAngle, cosAngle);
        particle->position = Vector3(curRadius * cosAngle, curRadius * sinAngle, 0.0f);
    }
    else if (isSphereEmitter)
    {
        float32 curRadius = 1.0f;
        float32 innerRadius = 0.0f;
        FillEmitterRadiuses(group, curRadius, innerRadius);

        float32 u = ParticlesRandom::VanDerCorputRnd(ind, 3);
        float32 v = ParticlesRandom::VanDerCorputRnd(ind, 5);
        float32 theta = PI_2 * u; // http://mathworld.wolfram.com/SpherePointPicking.html
        float32 phi = std::acos(2.0f * v - 1.0f);
        if (!group.emitter->generateOnSurface)
        {
            float32 rndRadiusNorm = std::sqrt(static_cast<float32>(GetEngineContext()->random->RandFloat())); // Better distribution on circle.
            curRadius = Lerp(innerRadius, curRadius, rndRadiusNorm);
        }
        float32 cosPhi = 0.0f;
        float32 sinPhi = 0.0f;
        SinCosFast(phi, sinPhi, cosPhi);
        float32 sinTheta = std::sin(theta);
        float32 radTimesSinTheta = curRadius * sinTheta;
        float32 x = radTimesSinTheta * cosPhi;
        float32 y = radTimesSinTheta * sinPhi;
        float32 z = curRadius * std::cos(theta);
        particle->position = Vector3(x, y, z);
    }

    //current emission vector and it's length
    Vector3 currEmissionVector(0, 0, 1);
    if (group.emitter->emissionVector)
        currEmissionVector = group.emitter->emissionVector->GetValue(group.time);
    float32 currEmissionPower = currEmissionVector.Length();

    Vector3 currVelVector = currEmissionVector;
    float32 currVelPower = currEmissionPower;
    bool hasCustomEmissionVector = group.emitter->emissionVelocityVector != nullptr;
    if (hasCustomEmissionVector)
    {
        currVelVector = group.emitter->emissionVelocityVector->GetValue(group.time);
        currVelPower = currVelVector.Length();
    }
    //calculate speed in emitter space not transformed by emission vector yet
    if ((isCircleEmitter && group.emitter->shockwaveMode != ParticleEmitter::SHOCKWAVE_DISABLED)
        || (isSphereEmitter && group.emitter->shockwaveMode == ParticleEmitter::SHOCKWAVE_NORMAL))
    {
        particle->speed = particle->position;
        float32 spl = particle->speed.SquareLength();
        if (spl > EPSILON)
        {
            particle->speed *= currVelPower / std::sqrt(spl);
        }
    }
    else if (isSphereEmitter && group.emitter->shockwaveMode == ParticleEmitter::SHOCKWAVE_HORIZONTAL)
    {
        float32 cosTheta = 0.0f;
        float32 sinTheta = 0.0f;
        if (group.emitter->emissionRange)
        {
            float32 rnd = ParticlesRandom::VanDerCorputRnd(ind, 3) * 2.0f - 1.0f;
            float32 diviation = rnd * DegToRad(group.emitter->emissionRange->GetValue(group.time)) * 0.5f;

            float32 theta = PI_05 + diviation;
            SinCosFast(theta, sinTheta, cosTheta);
        }
        else
            sinTheta = 1.0f; // theta = pi * 0.5

        float32 phi = static_cast<float32>(GetEngineContext()->random->RandFloat()) * PI_2;
        float32 cosPhi = 0.0f;
        float32 sinPhi = 0.0f;
        SinCosFast(phi, sinPhi, cosPhi);

        Vector3 newVel;
        newVel = Vector3(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
        newVel *= currVelPower;
        particle->speed = newVel;
    }
    else
    {
        if (group.emitter->emissionRange)
        {
            float32 theta = ParticlesRandom::VanDerCorputRnd(ind, 3) * DegToRad(group.emitter->emissionRange->GetValue(group.time)) * 0.5f;
            float32 phi = ParticlesRandom::VanDerCorputRnd(ind, 4) * PI_2;
            float32 sinTheta = std::sin(theta);
            particle->speed = Vector3(currVelPower * std::cos(phi) * sinTheta, currVelPower * std::sin(phi) * sinTheta, currVelPower * std::cos(theta));
        }
        else
            particle->speed = Vector3(0, 0, currVelPower);
    }

    //now transform position and speed by emissionVector and worldTransfrom rotations - preserving length
    Matrix3 newTransform(worldTransform);
    Matrix3 PIRotationAroundX;
    PIRotationAroundX.CreateRotation(Vector3(1, 0, 0), PI);
    if ((std::abs(currEmissionVector.x) < EPSILON) && (std::abs(currEmissionVector.y) < EPSILON))
    {
        if (currEmissionVector.z < 0)
        {
            particle->position = particle->position * PIRotationAroundX;

            if (!hasCustomEmissionVector)
                particle->speed = particle->speed * PIRotationAroundX;
        }
    }
    else
    {
        Matrix3 rotation = ParticleEffectSystemDetails::GenerateEmitterRotationMatrix(currEmissionVector, currEmissionPower);
        particle->position = particle->position * rotation;

        if (!hasCustomEmissionVector)
            particle->speed = particle->speed * rotation;
    }

    if (hasCustomEmissionVector)
    {
        if ((std::abs(currVelVector.x) < EPSILON) && (std::abs(currVelVector.y) < EPSILON))
        {
            if (currVelVector.z < 0)
                particle->speed = particle->speed * PIRotationAroundX;
        }
        else
        {
            particle->speed = particle->speed * ParticleEffectSystemDetails::GenerateEmitterRotationMatrix(currVelVector, currVelPower);
        }
    }
    particle->position += group.spawnPosition;
    TransformPerserveLength(particle->speed, newTransform);
    TransformPerserveLength(particle->position, newTransform); //note - from now emitter position is not effected by scale anymore (artist request)
}

void ParticleEffectSystem::SetGlobalExtertnalValue(const String& name, float32 value)
{
    globalExternalValues[name] = value;
    for (Vector<ParticleEffectComponent *>::iterator it = activeComponents.begin(), e = activeComponents.end(); it != e; ++it)
        (*it)->SetExtertnalValue(name, value);
}

float32 ParticleEffectSystem::GetGlobalExternalValue(const String& name)
{
    Map<String, float32>::iterator it = globalExternalValues.find(name);
    if (it != globalExternalValues.end())
        return (*it).second;
    else
        return 0.0f;
}

Map<String, float32> ParticleEffectSystem::GetGlobalExternals()
{
    return globalExternalValues;
}

void ParticleEffectSystem::SimulateEffect(ParticleEffectComponent* effect)
{
    static const float32 particleSystemFps = 30.0f;
    static const float32 delta = 0.0333f;
    uint32 frames = static_cast<uint32>(effect->GetStartFromTime() * particleSystemFps);
    for (uint32 i = 0; i < frames; ++i)
        UpdateEffect(effect, delta, delta);
}

void ParticleEffectSystem::ExtractGlobalForces(ParticleEffectComponent* effect)
{
    auto it = globalForces.find(effect);
    if (it != globalForces.end())
        globalForces.erase(it);
    for (uint32 i = 0; i < effect->GetEmittersCount(); ++i)
    {
        ParticleEmitterInstance* emitterInst = effect->GetEmitterInstance(i);
        ParticleEmitter* emitter = emitterInst->GetEmitter();
        for (ParticleLayer* layer : emitter->layers)
        {
            for (ParticleForce* force : layer->GetParticleForces())
            {
                if (force->isGlobal)
                {
                    if (force->worldAlign)
                        globalForces[effect].worldAlignForces.push_back(force);
                    else
                        globalForces[effect].effectAlignForces.push_back(force);
                }
            }
        }
    }
}

void ParticleEffectSystem::RemoveForcesFromGlobal(ParticleEffectComponent* effect)
{
    for (uint32 i = 0; i < effect->GetEmittersCount(); ++i)
    {
        ParticleEmitterInstance* emitterInst = effect->GetEmitterInstance(i);
        ParticleEmitter* emitter = emitterInst->GetEmitter();
        auto it = globalForces.find(effect);
        if (it != globalForces.end())
            globalForces.erase(it);
    }
}

void ParticleEffectSystem::FillEmitterRadiuses(const ParticleGroup& group, float32& radius, float32& innerRadius)
{
    if (group.emitter->radius)
        radius = group.emitter->radius->GetValue(group.time);

    if (group.emitter->innerRadius)
    {
        innerRadius = group.emitter->innerRadius->GetValue(group.time);
        innerRadius = Min(innerRadius, radius);
    }
}
}
