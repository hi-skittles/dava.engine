#include "Scene/SceneEditor2.h"
#include "Scene/System/EditorParticlesSystem.h"
#include "Scene/System/CollisionSystem.h"
#include "Scene/System/HoodSystem.h"
#include "Scene/SceneSignals.h"
#include "Main/mainwindow.h"

#include "Classes/Selection/Selection.h"

// framework
#include "Base/BaseTypes.h"
#include "Entity/Component.h"
#include "Particles/ParticleEmitter.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Math/Vector.h"

// particles-related commands
#include "Commands2/ParticleEditorCommands.h"
#include "Commands2/ParticleLayerCommands.h"
#include "Commands2/Base/RECommandNotificationObject.h"

namespace EditorParticlesSystemDetails
{
template <typename T>
inline const DAVA::Vector<T*>& GetForceVector(T* force, DAVA::ParticleLayer* layer);

template <>
inline const DAVA::Vector<DAVA::ParticleForce*>& GetForceVector(DAVA::ParticleForce* force, DAVA::ParticleLayer* layer)
{
    return layer->GetParticleForces();
}

template <>
inline const DAVA::Vector<DAVA::ParticleForceSimplified*>& GetForceVector(DAVA::ParticleForceSimplified* force, DAVA::ParticleLayer* layer)
{
    return layer->GetSimplifiedParticleForces();
}
}

EditorParticlesSystem::EditorParticlesSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
{
}

void EditorParticlesSystem::DrawDebugInfoForEffect(DAVA::Entity* effectEntity)
{
    DVASSERT(effectEntity != nullptr);

    SceneCollisionSystem* collisionSystem = static_cast<SceneEditor2*>(GetScene())->collisionSystem;

    DAVA::AABBox3 worldBox;
    DAVA::AABBox3 collBox = collisionSystem->GetBoundingBox(effectEntity);
    DVASSERT(!collBox.IsEmpty());
    collBox.GetTransformedBox(effectEntity->GetWorldTransform(), worldBox);
    DAVA::float32 radius = (collBox.max - collBox.min).Length() / 3;
    GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawIcosahedron(worldBox.GetCenter(), radius, DAVA::Color(0.9f, 0.9f, 0.9f, 0.35f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
}

void EditorParticlesSystem::DrawEmitter(DAVA::ParticleEmitterInstance* emitter, DAVA::Entity* owner, bool selected)
{
    DVASSERT((emitter != nullptr) && (owner != nullptr));

    SceneCollisionSystem* collisionSystem = ((SceneEditor2*)GetScene())->collisionSystem;

    DAVA::Vector3 center = emitter->GetSpawnPosition();
    TransformPerserveLength(center, DAVA::Matrix3(owner->GetWorldTransform()));
    center += owner->GetWorldTransform().GetTranslationVector();

    DAVA::AABBox3 boundingBox = collisionSystem->GetBoundingBox(owner);
    DVASSERT(!boundingBox.IsEmpty());
    DAVA::float32 radius = (boundingBox.max - boundingBox.min).Length() / 3;
    GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawIcosahedron(center, radius, DAVA::Color(1.0f, 1.0f, 1.0f, 0.5f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);

    if (selected)
    {
        DrawVectorArrow(emitter, center);

        switch (emitter->GetEmitter()->emitterType)
        {
        case DAVA::ParticleEmitter::EMITTER_ONCIRCLE_VOLUME:
        case DAVA::ParticleEmitter::EMITTER_ONCIRCLE_EDGES:
        {
            DrawSizeCircle(owner, emitter);
            break;
        }
        case DAVA::ParticleEmitter::EMITTER_SHOCKWAVE:
        {
            DrawSizeCircleShockWave(owner, emitter);
            break;
        }

        case DAVA::ParticleEmitter::EMITTER_RECT:
        {
            DrawSizeBox(owner, emitter);
            break;
        }

        default:
            break;
        }
    }
}

void EditorParticlesSystem::Draw()
{
    const SelectableGroup& selection = Selection::GetSelection();
    DAVA::Set<DAVA::ParticleEmitterInstance*> selectedEmitterInstances;
    for (auto instance : selection.ObjectsOfType<DAVA::ParticleEmitterInstance>())
    {
        selectedEmitterInstances.insert(instance);
    }

    for (DAVA::ParticleForce* force : selection.ObjectsOfType<DAVA::ParticleForce>())
        DrawParticleForces(force);

    for (auto entity : entities)
    {
        auto effect = entity->GetComponent<DAVA::ParticleEffectComponent>();
        if (effect != nullptr)
        {
            for (DAVA::uint32 i = 0, e = effect->GetEmittersCount(); i < e; ++i)
            {
                auto instance = effect->GetEmitterInstance(i);
                DrawEmitter(instance, entity, selectedEmitterInstances.count(instance) > 0);
            }
            DrawDebugInfoForEffect(entity);
        }
    }
}

void EditorParticlesSystem::DrawSizeCircleShockWave(DAVA::Entity* effectEntity, DAVA::ParticleEmitterInstance* emitter)
{
    DAVA::float32 time = GetEffectComponent(effectEntity)->GetCurrTime();
    DAVA::float32 emitterRadius = (emitter->GetEmitter()->radius) ? emitter->GetEmitter()->radius->GetValue(time) : 0.0f;
    DAVA::Vector3 emissionVector(0.0f, 0.0f, 1.0f);

    if (emitter->GetEmitter()->emissionVector)
    {
        DAVA::Matrix4 wMat = effectEntity->GetWorldTransform();
        wMat.SetTranslationVector(DAVA::Vector3(0.0f, 0.0f, 0.0f));
        emissionVector = emitter->GetEmitter()->emissionVector->GetValue(time) * wMat;
    }

    auto center = Selectable(emitter).GetWorldTransform().GetTranslationVector();

    auto drawer = GetScene()->GetRenderSystem()->GetDebugDrawer();
    drawer->DrawCircle(center, emissionVector, emitterRadius, 12, DAVA::Color(0.7f, 0.0f, 0.0f, 0.25f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
}

void EditorParticlesSystem::DrawSizeCircle(DAVA::Entity* effectEntity, DAVA::ParticleEmitterInstance* emitter)
{
    DAVA::float32 emitterRadius = 0.0f;
    DAVA::Vector3 emitterVector;
    DAVA::float32 time = GetEffectComponent(effectEntity)->GetCurrTime();

    if (emitter->GetEmitter()->radius)
    {
        emitterRadius = emitter->GetEmitter()->radius->GetValue(time);
    }

    if (emitter->GetEmitter()->emissionVector)
    {
        DAVA::Matrix4 wMat = effectEntity->GetWorldTransform();
        wMat.SetTranslationVector(DAVA::Vector3(0.0f, 0.0f, 0.0f));
        emitterVector = emitter->GetEmitter()->emissionVector->GetValue(time) * wMat;
    }

    auto center = Selectable(emitter).GetWorldTransform().GetTranslationVector();

    auto drawer = GetScene()->GetRenderSystem()->GetDebugDrawer();
    drawer->DrawCircle(center, emitterVector, emitterRadius, 12,
                       DAVA::Color(0.7f, 0.0f, 0.0f, 0.25f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
}

void EditorParticlesSystem::DrawSizeBox(DAVA::Entity* effectEntity, DAVA::ParticleEmitterInstance* emitter)
{
    // Default value of emitter size
    DAVA::Vector3 emitterSize;

    DAVA::float32 time = GetEffectComponent(effectEntity)->GetCurrTime();

    if (emitter->GetEmitter()->size)
    {
        emitterSize = emitter->GetEmitter()->size->GetValue(time);
    }

    DAVA::Matrix4 wMat = effectEntity->GetWorldTransform();

    wMat.SetTranslationVector(Selectable(emitter).GetWorldTransform().GetTranslationVector());

    DAVA::RenderHelper* drawer = GetScene()->GetRenderSystem()->GetDebugDrawer();
    drawer->DrawAABoxTransformed(DAVA::AABBox3(-0.5f * emitterSize, 0.5f * emitterSize), wMat,
                                 DAVA::Color(0.7f, 0.0f, 0.0f, 0.25f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
}

void EditorParticlesSystem::DrawVectorArrow(DAVA::ParticleEmitterInstance* emitter, DAVA::Vector3 center)
{
    DAVA::ParticleEffectComponent* effect = emitter->GetOwner();
    if (effect == nullptr)
        return;

    DAVA::Vector3 emitterVector(0.0f, 0.0f, 1.0f);
    if (emitter->GetEmitter()->emissionVector)
    {
        emitterVector = emitter->GetEmitter()->emissionVector->GetValue(effect->GetCurrTime());
        emitterVector.Normalize();
    }
    DAVA::Vector3 emissionVelocityVector(0.0f, 0.0f, 1.0f);
    if (emitter->GetEmitter()->emissionVelocityVector)
    {
        emissionVelocityVector = emitter->GetEmitter()->emissionVelocityVector->GetValue(effect->GetCurrTime());
        DAVA::float32 sqrLen = emissionVelocityVector.SquareLength();
        if (sqrLen > DAVA::EPSILON)
            emissionVelocityVector /= std::sqrt(sqrLen);
    }

    DAVA::float32 scale = 1.0f;
    HoodSystem* hoodSystem = ((SceneEditor2*)GetScene())->hoodSystem;
    if (hoodSystem != nullptr)
    {
        scale = hoodSystem->GetScale();
    }

    DAVA::float32 arrowSize = scale;
    DAVA::float32 arrowBaseSize = 5.0f;
    emitterVector = (emitterVector * arrowBaseSize * scale);

    DAVA::Matrix4 wMat = effect->GetEntity()->GetWorldTransform();
    wMat.SetTranslationVector(DAVA::Vector3(0, 0, 0));
    TransformPerserveLength(emitterVector, wMat);

    emissionVelocityVector *= arrowBaseSize * scale;
    TransformPerserveLength(emissionVelocityVector, wMat);

    GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawArrow(center, center + emitterVector, arrowSize,
                                                               DAVA::Color(0.7f, 0.0f, 0.0f, 0.25f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);

    if (emitter->GetEmitter()->emissionVelocityVector)
        GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawArrow(center, center + emissionVelocityVector, arrowSize,
                                                                   DAVA::Color(0.0f, 0.0f, 0.7f, 0.25f), DAVA::RenderHelper::DRAW_SOLID_DEPTH);
}

void EditorParticlesSystem::DrawParticleForces(DAVA::ParticleForce* force)
{
    using namespace DAVA;
    using ForceType = ParticleForce::eType;

    if (force->type == ForceType::GRAVITY)
        return;

    RenderHelper* drawer = GetScene()->GetRenderSystem()->GetDebugDrawer();
    DAVA::ParticleLayer* layer = GetForceOwner(force);
    if (layer == nullptr)
        return;

    DAVA::ParticleEmitterInstance* emitterInstance = GetRootEmitterLayerOwner(layer);
    DAVA::ParticleEffectComponent* effectComponent = emitterInstance->GetOwner();
    DAVA::Entity* entity = effectComponent->GetEntity();
    if (force->type == ForceType::VORTEX || force->type == ForceType::WIND || force->type == ForceType::PLANE_COLLISION)
    {
        float32 scale = 1.0f;
        HoodSystem* hoodSystem = static_cast<SceneEditor2*>(GetScene())->hoodSystem;
        if (hoodSystem != nullptr)
            scale = hoodSystem->GetScale();

        float32 arrowSize = scale;
        float32 arrowBaseSize = 5.0f;
        Vector3 emitterVector = force->direction;
        Vector3 center;
        if (force->worldAlign)
        {
            center = entity->GetWorldTransform().GetTranslationVector() + force->position;
        }
        else
        {
            Matrix4 wMat = entity->GetWorldTransform();
            emitterVector = emitterVector * Matrix3(wMat);
            center = force->position * wMat;
        }
        emitterVector.Normalize();
        emitterVector *= arrowBaseSize * scale;

        drawer->DrawArrow(center, center + emitterVector, arrowSize, Color(0.7f, 0.7f, 0.0f, 0.35f), RenderHelper::DRAW_SOLID_DEPTH);
    }

    if (force->type == ForceType::PLANE_COLLISION)
    {
        Matrix4 wMat = entity->GetWorldTransform();
        Vector3 forcePosition;
        Vector3 wNormal;
        if (force->worldAlign)
        {
            forcePosition = wMat.GetTranslationVector() + force->position;
            wNormal = force->direction;
        }
        else
        {
            wNormal = force->direction * Matrix3(wMat);
            forcePosition = force->position;
            forcePosition = force->position * wMat;
        }
        wNormal.Normalize();
        Vector3 cV(0.0f, 0.0f, 1.0f);
        if (1.0f - Abs(cV.DotProduct(wNormal)) < EPSILON)
            cV = Vector3(1.0f, 0.0f, 0.0f);
        Matrix4 transform;
        transform.BuildLookAtMatrix(forcePosition, forcePosition + wNormal, cV);
        transform.Inverse();

        float32 bbsize = force->planeScale * 0.5f;
        drawer->DrawAABoxTransformed(AABBox3(Vector3(-bbsize, -bbsize, -0.1f), Vector3(bbsize, bbsize, 0.01f)), transform,
                                     Color(0.0f, 0.7f, 0.7f, 0.25f), RenderHelper::DRAW_SOLID_DEPTH);
        drawer->DrawAABoxTransformed(AABBox3(Vector3(-bbsize, -bbsize, -0.1f), Vector3(bbsize, bbsize, 0.01f)), transform,
                                     Color(0.0f, 0.35f, 0.35f, 0.35f), RenderHelper::DRAW_WIRE_DEPTH);
    }
    else if (force->type == ForceType::POINT_GRAVITY)
    {
        float32 radius = force->pointGravityRadius;
        Vector3 translation;
        if (force->worldAlign)
            translation = entity->GetWorldTransform().GetTranslationVector() + force->position;
        else
            translation = Selectable(force).GetWorldTransform().GetTranslationVector();

        drawer->DrawIcosahedron(translation, radius, Color(0.0f, 0.3f, 0.7f, 0.25f), RenderHelper::DRAW_SOLID_DEPTH);
        drawer->DrawIcosahedron(translation, radius, Color(0.0f, 0.15f, 0.35f, 0.35f), RenderHelper::DRAW_WIRE_DEPTH);
    }

    if (force->isInfinityRange)
        return;
    if (force->GetShape() == ParticleForce::eShape::BOX)
    {
        Matrix4 wMat = entity->GetWorldTransform();
        if (force->worldAlign)
        {
            Vector3 translation = wMat.GetTranslationVector();
            wMat = Matrix4::IDENTITY;
            wMat.SetTranslationVector(translation + force->position);
        }
        else
            wMat.SetTranslationVector(Selectable(force).GetWorldTransform().GetTranslationVector());

        drawer->DrawAABoxTransformed(AABBox3(-force->GetHalfBoxSize(), force->GetHalfBoxSize()), wMat,
                                     Color(0.0f, 0.7f, 0.3f, 0.25f), RenderHelper::DRAW_SOLID_DEPTH);

        drawer->DrawAABoxTransformed(AABBox3(-force->GetHalfBoxSize(), force->GetHalfBoxSize()), wMat,
                                     Color(0.0f, 0.35f, 0.15f, 0.35f), RenderHelper::DRAW_WIRE_DEPTH);
    }
    else if (force->GetShape() == ParticleForce::eShape::SPHERE)
    {
        Matrix4 wMat = Selectable(force).GetWorldTransform();
        if (force->worldAlign)
        {
            Vector3 translation = entity->GetWorldTransform().GetTranslationVector();
            wMat = Matrix4::IDENTITY;
            wMat.SetTranslationVector(translation + force->position);
        }
        float32 radius = force->GetRadius();
        drawer->DrawIcosahedron(wMat.GetTranslationVector(), radius, Color(0.0f, 0.7f, 0.3f, 0.25f), RenderHelper::DRAW_SOLID_DEPTH);
        drawer->DrawIcosahedron(wMat.GetTranslationVector(), radius, Color(0.0f, 0.35f, 0.15f, 0.35f), RenderHelper::DRAW_WIRE_DEPTH);
    }
}

void EditorParticlesSystem::AddEntity(DAVA::Entity* entity)
{
    entities.push_back(entity);
}

void EditorParticlesSystem::RemoveEntity(DAVA::Entity* entity)
{
    DAVA::FindAndRemoveExchangingWithLast(entities, entity);
}

void EditorParticlesSystem::PrepareForRemove()
{
    entities.clear();
}

void EditorParticlesSystem::RestartParticleEffects()
{
    for (DAVA::Entity* entity : entities)
    {
        DAVA::ParticleEffectComponent* effectComponent = DAVA::GetEffectComponent(entity);
        DVASSERT(effectComponent);
        if (!effectComponent->IsStopped())
        {
            effectComponent->Restart();
        }
    }
}

DAVA::ParticleEffectComponent* EditorParticlesSystem::GetEmitterOwner(DAVA::ParticleEmitterInstance* wantedEmitter) const
{
    DAVA::ParticleEffectComponent* component = wantedEmitter->GetOwner();
    if (component != nullptr)
    {
        return component;
    }

    using TFunctor = DAVA::Function<bool(DAVA::ParticleEmitterInstance*)>;
    TFunctor lookUpEmitter = [&lookUpEmitter, &wantedEmitter](DAVA::ParticleEmitterInstance* emitter) {
        if (emitter == wantedEmitter)
        {
            return true;
        }

        for (DAVA::ParticleLayer* layer : emitter->GetEmitter()->layers)
        {
            if (layer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
            {
                DVASSERT(layer->innerEmitter != nullptr);
                if (lookUpEmitter(layer->innerEmitter) == true)
                {
                    return true;
                }
            }
        }

        return false;
    };

    for (DAVA::Entity* entity : entities)
    {
        DAVA::ParticleEffectComponent* effectComponent = DAVA::GetEffectComponent(entity);
        DAVA::uint32 emittersCount = effectComponent->GetEmittersCount();
        for (DAVA::uint32 id = 0; id < emittersCount; ++id)
        {
            DAVA::ParticleEmitterInstance* emitterInstance = effectComponent->GetEmitterInstance(id);
            DVASSERT(emitterInstance != nullptr);

            bool found = lookUpEmitter(emitterInstance);
            if (found == true)
            {
                return effectComponent;
            }
        }
    }

    return nullptr;
}

template <typename T>
DAVA::ParticleLayer* EditorParticlesSystem::GetForceOwner(T* force) const
{
    DAVA::Function<DAVA::ParticleLayer*(DAVA::ParticleEmitterInstance*, T*)> getForceOwner = [&getForceOwner](DAVA::ParticleEmitterInstance* emitter, T* force) -> DAVA::ParticleLayer*
    {
        for (DAVA::ParticleLayer* layer : emitter->GetEmitter()->layers)
        {
            if (layer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
            {
                DVASSERT(layer->innerEmitter != nullptr);
                DAVA::ParticleLayer* foundLayer = getForceOwner(layer->innerEmitter, force);
                if (foundLayer != nullptr)
                {
                    return foundLayer;
                }
            }
            const DAVA::Vector<T*>& forces = EditorParticlesSystemDetails::GetForceVector(force, layer);
            if (std::find(forces.begin(), forces.end(), force) != forces.end())
            {
                return layer;
            }
        }

        return nullptr;
    };

    for (DAVA::Entity* entity : entities)
    {
        DAVA::ParticleEffectComponent* effectComponent = DAVA::GetEffectComponent(entity);
        DAVA::uint32 emittersCount = effectComponent->GetEmittersCount();
        for (DAVA::uint32 id = 0; id < emittersCount; ++id)
        {
            DAVA::ParticleEmitterInstance* emitterInstance = effectComponent->GetEmitterInstance(id);
            DVASSERT(emitterInstance != nullptr);

            DAVA::ParticleLayer* owner = getForceOwner(emitterInstance, force);
            if (owner != nullptr)
            {
                return owner;
            }
        }
    }

    return nullptr;
}

DAVA::ParticleEmitterInstance* EditorParticlesSystem::GetRootEmitterLayerOwner(DAVA::ParticleLayer* layer) const
{
    DAVA::Function<bool(DAVA::ParticleEmitterInstance*, DAVA::ParticleLayer*)> hasLayerOwner = [&hasLayerOwner](DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer) -> bool
    {
        for (DAVA::ParticleLayer* l : emitter->GetEmitter()->layers)
        {
            if (l == layer)
            {
                return true;
            }

            if (l->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
            {
                DVASSERT(l->innerEmitter != nullptr);
                bool found = hasLayerOwner(l->innerEmitter, layer);
                if (found)
                {
                    return true;
                }
            }
        }

        return false;
    };

    for (DAVA::Entity* entity : entities)
    {
        DAVA::ParticleEffectComponent* effectComponent = DAVA::GetEffectComponent(entity);
        DAVA::uint32 emittersCount = effectComponent->GetEmittersCount();
        for (DAVA::uint32 id = 0; id < emittersCount; ++id)
        {
            DAVA::ParticleEmitterInstance* emitterInstance = effectComponent->GetEmitterInstance(id);
            DVASSERT(emitterInstance != nullptr);

            if (hasLayerOwner(emitterInstance, layer))
            {
                return emitterInstance;
            }
        }
    }

    return nullptr;
}

DAVA::ParticleEmitterInstance* EditorParticlesSystem::GetDirectEmitterLayerOwner(DAVA::ParticleLayer* layer) const
{
    using TFunctor = DAVA::Function<DAVA::ParticleEmitterInstance*(DAVA::ParticleEmitterInstance*, DAVA::ParticleLayer*)>;
    TFunctor lookUpEmitter = [&lookUpEmitter](DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer) -> DAVA::ParticleEmitterInstance* {
        for (DAVA::ParticleLayer* l : emitter->GetEmitter()->layers)
        {
            if (l == layer)
            {
                return emitter;
            }

            if (l->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
            {
                DVASSERT(l->innerEmitter != nullptr);
                DAVA::ParticleEmitterInstance* result = lookUpEmitter(l->innerEmitter, layer);
                if (result != nullptr)
                {
                    return result;
                }
            }
        }

        return nullptr;
    };

    for (DAVA::Entity* entity : entities)
    {
        DAVA::ParticleEffectComponent* effectComponent = DAVA::GetEffectComponent(entity);
        DAVA::uint32 emittersCount = effectComponent->GetEmittersCount();
        for (DAVA::uint32 id = 0; id < emittersCount; ++id)
        {
            DAVA::ParticleEmitterInstance* emitterInstance = effectComponent->GetEmitterInstance(id);
            DVASSERT(emitterInstance != nullptr);

            DAVA::ParticleEmitterInstance* wantedEmitter = lookUpEmitter(emitterInstance, layer);

            if (wantedEmitter != nullptr)
            {
                return wantedEmitter;
            }
        }
    }

    return nullptr;
}

void EditorParticlesSystem::ProcessCommand(const RECommandNotificationObject& commandNotification)
{
    SceneEditor2* activeScene = static_cast<SceneEditor2*>(GetScene());
    auto processSingleCommand = [&activeScene, this](const RECommand* command, bool redo)
    {
        switch (command->GetID())
        {
        case CMDID_PARTICLE_EMITTER_UPDATE:
        {
            const CommandUpdateEmitter* castedCmd = static_cast<const CommandUpdateEmitter*>(command);
            SceneSignals::Instance()->EmitParticleEmitterValueChanged(activeScene, castedCmd->GetEmitterInstance());
            break;
        }

        case CMDID_PARTICLE_LAYER_UPDATE:
        {
            const CommandUpdateParticleLayerBase* castedCmd = static_cast<const CommandUpdateParticleLayerBase*>(command);
            SceneSignals::Instance()->EmitParticleLayerValueChanged(activeScene, castedCmd->GetLayer());
            break;
        }
        case CMDID_PARTICLE_LAYER_CHANGED_MATERIAL_VALUES:
        {
            EmitValueChanged<CommandChangeLayerMaterialProperties>(command, activeScene);
            break;
        }
        case CMDID_PARTICLE_LAYER_CHANGED_FLOW_VALUES:
        {
            EmitValueChanged<CommandChangeFlowProperties>(command, activeScene);
            break;
        }
        case CMDID_PARTICLE_LAYER_CHANGED_NOISE_VALUES:
        {
            EmitValueChanged<CommandChangeNoiseProperties>(command, activeScene);
            break;
        }
        case CMDID_PARTICLE_LAYER_CHANGED_FRES_TO_ALPHA_VALUES:
        {
            EmitValueChanged<CommandChangeFresnelToAlphaProperties>(command, activeScene);
            break;
        }
        case CMDID_PARTICLE_LAYER_CHANGED_STRIPE_VALUES:
        {
            EmitValueChanged<CommandChangeParticlesStripeProperties>(command, activeScene);
            break;
        }
        case CMDID_PARTICLE_LAYER_CHANGED_ALPHA_REMAP:
        {
            EmitValueChanged<CommandChangeAlphaRemapProperties>(command, activeScene);
            break;
        }
        case CMDID_PARTICLE_LAYER_CHANGED_THREE_POINT_GRADIENT:
        {
            EmitValueChanged<CommandChangeThreePointGradientProperties>(command, activeScene);
            break;
        }

        case CMDID_PARTILCE_LAYER_UPDATE_TIME:
        case CMDID_PARTICLE_LAYER_UPDATE_ENABLED:
        {
            const CommandUpdateParticleLayerBase* castedCmd = static_cast<const CommandUpdateParticleLayerBase*>(command);
            SceneSignals::Instance()->EmitParticleLayerValueChanged(activeScene, castedCmd->GetLayer());
            break;
        }

        case CMDID_PARTICLE_SIMPLIFIED_FORCE_UPDATE:
        {
            const CommandUpdateParticleSimplifiedForce* castedCmd = static_cast<const CommandUpdateParticleSimplifiedForce*>(command);
            SceneSignals::Instance()->EmitParticleForceValueChanged(activeScene, castedCmd->GetLayer(), castedCmd->GetForceIndex());
            break;
        }
        case CMDID_PARTICLE_FORCE_UPDATE:
        {
            const CommandUpdateParticleForce* castedCmd = static_cast<const CommandUpdateParticleForce*>(command);
            SceneSignals::Instance()->EmitParticleDragForceValueChanged(activeScene, castedCmd->GetLayer(), castedCmd->GetForceIndex());
        }

        case CMDID_PARTICLE_EFFECT_START_STOP:
        {
            const CommandStartStopParticleEffect* castedCmd = static_cast<const CommandStartStopParticleEffect*>(command);
            SceneSignals::Instance()->EmitParticleEffectStateChanged(activeScene, castedCmd->GetEntity(), castedCmd->GetStarted());
            break;
        }

        case CMDID_PARTICLE_EFFECT_RESTART:
        {
            const CommandRestartParticleEffect* castedCmd = static_cast<const CommandRestartParticleEffect*>(command);

            // An effect was stopped and then started.
            SceneSignals::Instance()->EmitParticleEffectStateChanged(activeScene, castedCmd->GetEntity(), false);
            SceneSignals::Instance()->EmitParticleEffectStateChanged(activeScene, castedCmd->GetEntity(), true);
            break;
        }

        case CMDID_PARTICLE_EMITTER_LOAD_FROM_YAML:
        {
            const CommandLoadParticleEmitterFromYaml* castedCmd = static_cast<const CommandLoadParticleEmitterFromYaml*>(command);
            SceneSignals::Instance()->EmitParticleEmitterLoaded(activeScene, castedCmd->GetEmitterInstance());
            break;
        }

        case CMDID_PARTICLE_EMITTER_SAVE_TO_YAML:
        {
            const CommandSaveParticleEmitterToYaml* castedCmd = static_cast<const CommandSaveParticleEmitterToYaml*>(command);
            SceneSignals::Instance()->EmitParticleEmitterSaved(activeScene, castedCmd->GetEmitterInstance());
            break;
        }

        case CMDID_PARTICLE_INNER_EMITTER_LOAD_FROM_YAML:
        {
            const CommandLoadInnerParticleEmitterFromYaml* castedCmd = static_cast<const CommandLoadInnerParticleEmitterFromYaml*>(command);
            SceneSignals::Instance()->EmitParticleEmitterLoaded(activeScene, castedCmd->GetEmitterInstance());
            break;
        }

        case CMDID_PARTICLE_INNER_EMITTER_SAVE_TO_YAML:
        {
            const CommandSaveInnerParticleEmitterToYaml* castedCmd = static_cast<const CommandSaveInnerParticleEmitterToYaml*>(command);
            SceneSignals::Instance()->EmitParticleEmitterSaved(activeScene, castedCmd->GetEmitterInstance());
            break;
        }

        case CMDID_PARTICLE_EMITTER_LAYER_ADD:
        {
            const CommandAddParticleEmitterLayer* castedCmd = static_cast<const CommandAddParticleEmitterLayer*>(command);
            SceneSignals::Instance()->EmitParticleLayerAdded(activeScene, castedCmd->GetParentEmitter(), castedCmd->GetCreatedLayer());
            break;
        }
        // Return to this code when implementing Layer popup menus.
        /*
        case CMDID_REMOVE_PARTICLE_EMITTER_LAYER:
        {
        const CommandRemoveParticleEmitterLayer* castedCmd = static_cast<const CommandRemoveParticleEmitterLayer*>(command);
        SceneSignals::Instance()->EmitParticleLayerRemoved(activeScene, castedCmd->GetEmitter());
        break;
        }
        */
        default:
            break;
        }
    };

    static const DAVA::Vector<DAVA::uint32> commandIDs =
    {
      CMDID_PARTICLE_EMITTER_UPDATE, CMDID_PARTICLE_LAYER_UPDATE, CMDID_PARTICLE_LAYER_CHANGED_MATERIAL_VALUES, CMDID_PARTICLE_LAYER_CHANGED_FLOW_VALUES, CMDID_PARTICLE_LAYER_CHANGED_NOISE_VALUES, CMDID_PARTICLE_LAYER_CHANGED_FRES_TO_ALPHA_VALUES, CMDID_PARTICLE_LAYER_CHANGED_STRIPE_VALUES, CMDID_PARTICLE_LAYER_CHANGED_ALPHA_REMAP,
      CMDID_PARTICLE_LAYER_CHANGED_THREE_POINT_GRADIENT, CMDID_PARTILCE_LAYER_UPDATE_TIME, CMDID_PARTICLE_LAYER_UPDATE_ENABLED, CMDID_PARTICLE_FORCE_UPDATE, CMDID_PARTICLE_SIMPLIFIED_FORCE_UPDATE,
      CMDID_PARTICLE_EFFECT_START_STOP, CMDID_PARTICLE_EFFECT_RESTART, CMDID_PARTICLE_EMITTER_LOAD_FROM_YAML,
      CMDID_PARTICLE_EMITTER_SAVE_TO_YAML,
      CMDID_PARTICLE_INNER_EMITTER_LOAD_FROM_YAML, CMDID_PARTICLE_INNER_EMITTER_SAVE_TO_YAML,
      //            CMDID_REMOVE_PARTICLE_EMITTER_LAYER,
      CMDID_PARTICLE_EMITTER_LAYER_ADD
    };

    if (commandNotification.MatchCommandIDs(commandIDs))
    {
        commandNotification.ExecuteForAllCommands(processSingleCommand);
    }
}

template DAVA::ParticleLayer* EditorParticlesSystem::GetForceOwner<DAVA::ParticleForce>(DAVA::ParticleForce* force) const;
template DAVA::ParticleLayer* EditorParticlesSystem::GetForceOwner<DAVA::ParticleForceSimplified>(DAVA::ParticleForceSimplified* force) const;
