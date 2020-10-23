#include "Classes/SceneTree/Private/SceneTreeItemTraits.h"
#include "Classes/SceneTree/Private/SceneTreeRoles.h"
#include "Classes/Application/REMeta.h"
#include "Classes/Commands2/ParticleEditorCommands.h"
#include "Classes/DragNDropSupport/ReflectedMimeData.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/System/EditorParticlesSystem.h"
#include "Classes/Qt/Scene/System/StructureSystem.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Selection/SelectableGroup.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Utils/Utils.h>

#include <Debug/DVAssert.h>
#include <Logger/Logger.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/StaticOcclusionComponent.h>
#include <Scene3D/Components/TextComponent.h>
#include <Scene3D/Components/UserComponent.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>

#include <QBrush>
#include <QColor>

BaseSceneTreeTraits::BaseSceneTreeTraits(DAVA::TArc::ContextAccessor* accessor_)
    : accessor(accessor_)
{
}

QString BaseSceneTreeTraits::GetTooltip(const Selectable& object) const
{
    return GetName(object);
}

Qt::ItemFlags BaseSceneTreeTraits::GetItemFlags(const Selectable& object, Qt::ItemFlags defaultFlags) const
{
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
}

QVariant BaseSceneTreeTraits::GetValue(const Selectable& object, int itemRole) const
{
    return QVariant();
}

bool BaseSceneTreeTraits::SetValue(const Selectable& object, const QVariant& value, int itemRole,
                                   SceneEditor2* scene) const
{
    return false;
}

EntityTraits::EntityTraits(DAVA::TArc::ContextAccessor* accessor)
    : BaseSceneTreeTraits(accessor)
{
}

QIcon EntityTraits::GetIcon(const Selectable& object) const
{
    using namespace DAVA::TArc;

    DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* entity = object.Cast<DAVA::Entity>();

    if (nullptr != entity->GetComponent<DAVA::StaticOcclusionComponent>())
    {
        return SharedIcon(":/QtIcons/so.png");
    }
    if (nullptr != DAVA::GetEffectComponent(entity))
    {
        return SharedIcon(":/QtIcons/effect.png");
    }
    else if (nullptr != DAVA::GetLandscape(entity))
    {
        return SharedIcon(":/QtIcons/heightmapeditor.png");
    }
    else if (nullptr != GetLodComponent(entity))
    {
        return SharedIcon(":/QtIcons/lod_object.png");
    }
    else if (nullptr != GetSwitchComponent(entity))
    {
        return SharedIcon(":/QtIcons/switch.png");
    }
    else if (nullptr != DAVA::GetVegetation(entity))
    {
        return SharedIcon(":/QtIcons/grass.png");
    }
    else if (nullptr != DAVA::GetSkeletonComponent(entity))
    {
        return SharedIcon(":/QtIcons/skinned_object.png");
    }
    else if (nullptr != DAVA::GetRenderObject(entity))
    {
        return SharedIcon(":/QtIcons/render_object.png");
    }
    else if (nullptr != entity->GetComponent<DAVA::UserComponent>())
    {
        return SharedIcon(":/QtIcons/user_object.png");
    }
    else if (nullptr != DAVA::GetCamera(entity))
    {
        return SharedIcon(":/QtIcons/camera.png");
    }
    else if (nullptr != DAVA::GetLight(entity))
    {
        return SharedIcon(":/QtIcons/light.png");
    }
    else if (nullptr != DAVA::GetWindComponent(entity))
    {
        return SharedIcon(":/QtIcons/wind.png");
    }
    else if (nullptr != DAVA::GetPathComponent(entity))
    {
        return SharedIcon(":/QtIcons/path.png");
    }
    else if (0 != entity->GetComponentCount<DAVA::TextComponent>())
    {
        return SharedIcon(":/QtIcons/text_component.png");
    }

    return DAVA::TArc::SharedIcon(":/QtIcons/node.png");
}

QString EntityTraits::GetName(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* entity = object.Cast<DAVA::Entity>();

    DAVA::FastName name = entity->GetName();
    return QString(name.c_str());
}

DAVA::int32 EntityTraits::GetChildrenCount(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* entity = object.Cast<DAVA::Entity>();
    DAVA::int32 entityChildrenCount = entity->GetChildrenCount();
    DAVA::int32 particleEffectsCount = 0;
    DAVA::ParticleEffectComponent* effectComponent = DAVA::GetParticleEffectComponent(entity);
    if (effectComponent != nullptr)
    {
        particleEffectsCount = effectComponent->GetEmittersCount();
    }

    return entityChildrenCount + particleEffectsCount;
}

void EntityTraits::BuildUnfetchedList(const Selectable& object,
                                      const DAVA::Function<bool(const Selectable&)>& isFetchedFn,
                                      DAVA::Vector<DAVA::int32>& unfetchedIndexes) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* entity = object.Cast<DAVA::Entity>();

    DAVA::int32 childCount = entity->GetChildrenCount();
    for (DAVA::int32 childIndex = 0; childIndex < childCount; ++childIndex)
    {
        DAVA::Entity* childEntity = entity->GetChild(childIndex);
        if (isFetchedFn(Selectable(DAVA::Any(childEntity))) == false)
        {
            unfetchedIndexes.push_back(childIndex);
        }
    }

    DAVA::ParticleEffectComponent* effectComponent = DAVA::GetParticleEffectComponent(entity);
    if (effectComponent != nullptr)
    {
        for (DAVA::uint32 emitterIndex = 0; emitterIndex < effectComponent->GetEmittersCount(); ++emitterIndex)
        {
            DAVA::ParticleEmitterInstance* emitterInstance = effectComponent->GetEmitterInstance(emitterIndex);
            if (isFetchedFn(Selectable(DAVA::Any(emitterInstance))) == false)
            {
                unfetchedIndexes.push_back(childCount + emitterIndex);
            }
        }
    }
}

void EntityTraits::FetchMore(const Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                             const DAVA::Function<void(DAVA::int32, const Selectable&)>& fetchCallback) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* entity = object.Cast<DAVA::Entity>();

    DAVA::int32 entityChildrenCount = entity->GetChildrenCount();
    DAVA::ParticleEffectComponent* effectComponent = DAVA::GetParticleEffectComponent(entity);

    for (DAVA::int32 index : unfetchedIndexes)
    {
        if (index < entity->GetChildrenCount())
        {
            DAVA::Entity* child = entity->GetChild(index);
            fetchCallback(index, Selectable(DAVA::Any(child)));
        }
        else
        {
            DVASSERT(effectComponent != nullptr);
            DAVA::int32 emitterIndex = index - entityChildrenCount;
            DVASSERT(emitterIndex >= 0);
            DAVA::uint32 unsignedEmitterIndex = static_cast<DAVA::uint32>(emitterIndex);
            DVASSERT(unsignedEmitterIndex < effectComponent->GetEmittersCount());
            DAVA::ParticleEmitterInstance* emitterInstance = effectComponent->GetEmitterInstance(unsignedEmitterIndex);
            fetchCallback(index, Selectable(DAVA::Any(emitterInstance)));
        }
    }
}

Qt::ItemFlags EntityTraits::GetItemFlags(const Selectable& object, Qt::ItemFlags defaultFlags) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* entity = object.Cast<DAVA::Entity>();
    if (entity->GetParent() == nullptr)
    {
        return Qt::ItemIsDropEnabled | defaultFlags;
    }
    if (IsEditorLocalEntity(entity) == false && entity->GetLocked() == false && IsDragNDropAllow(entity) == true)
    {
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    }

    return defaultFlags;
}

QVariant EntityTraits::GetValue(const Selectable& object, int itemRole) const
{
    if (itemRole == ToItemRoleCast(eSceneTreeRoles::RightAlignedDecorationRole))
    {
        QVector<QIcon> result;
        DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
        DAVA::Entity* entity = object.Cast<DAVA::Entity>();

        if (entity->GetLocked())
        {
            result.push_back(DAVA::TArc::SharedIcon(":/QtIcons/locked.png"));
        }

        DAVA::Camera* camera = DAVA::GetCamera(entity);
        if (camera != nullptr)
        {
            DAVA::TArc::DataContext* ctx = accessor->GetActiveContext();
            if (ctx != nullptr)
            {
                DAVA::Scene* scene = ctx->GetData<SceneData>()->GetScene().Get();
                if (scene->GetCurrentCamera() == camera)
                {
                    result.push_back(DAVA::TArc::SharedIcon(":/QtIcons/eye.png"));
                }
            }
        }

        return QVariant::fromValue(result);
    }
    else if (itemRole == ToItemRoleCast(eSceneTreeRoles::ForegroundAlphaRole))
    {
        DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
        DAVA::Entity* entity = object.Cast<DAVA::Entity>();

        if (IsEditorLocalEntity(entity) == true)
        {
            return QVariant::fromValue(100);
        }
    }
    else if (itemRole == ToItemRoleCast(eSceneTreeRoles::FilterDataRole))
    {
        DVASSERT(object.CanBeCastedTo<DAVA::Entity>());
        DAVA::Entity* entity = object.Cast<DAVA::Entity>();

        return QString::number(entity->GetID());
    }

    return QVariant();
}

bool EntityTraits::CanBeDropped(const ReflectedMimeData* mimeData, Qt::DropAction action,
                                const Selectable& parent, DAVA::int32 positionInParent) const
{
    if (action != Qt::MoveAction)
    {
        return false;
    }

    DVASSERT(parent.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* newParentEntity = parent.Cast<DAVA::Entity>();

    if (IsEditorLocalEntity(newParentEntity) == true || newParentEntity->GetLocked() == true)
    {
        return false;
    }

    DAVA::Vector<DAVA::Entity*> objects = mimeData->GetObjects<DAVA::Entity>();
    for (DAVA::Entity* droppedEntity : objects)
    {
        if (CanBeEntityReparent(droppedEntity, newParentEntity) == true)
        {
            return true;
        }
    }

    DAVA::Vector<DAVA::ParticleEmitterInstance*> emitters = mimeData->GetObjects<DAVA::ParticleEmitterInstance>();
    if (emitters.empty() == false && DAVA::GetParticleEffectComponent(newParentEntity) != nullptr)
    {
        return true;
    }

    return false;
}

bool EntityTraits::Drop(const ReflectedMimeData* mimeData, Qt::DropAction action,
                        const Selectable& parent, DAVA::int32 positionInParent, SceneEditor2* scene) const
{
    if (action != Qt::MoveAction)
    {
        return false;
    }

    DVASSERT(parent.CanBeCastedTo<DAVA::Entity>());
    DAVA::Entity* newParentEntity = parent.Cast<DAVA::Entity>();
    DVASSERT(IsEditorLocalEntity(newParentEntity) == false);
    DVASSERT(newParentEntity->GetLocked() == false);

    // extract entities for move
    {
        DAVA::Vector<DAVA::Entity*> entities = mimeData->GetObjects<DAVA::Entity>();
        if (entities.empty() == false)
        {
            DAVA::Entity* insertBefore = nullptr;
            if (positionInParent < newParentEntity->GetChildrenCount())
            {
                insertBefore = newParentEntity->GetChild(positionInParent);
            }

            DAVA::Vector<Selectable> objects;
            objects.reserve(entities.size());
            std::transform(entities.begin(), entities.end(), std::back_inserter(objects), [](const DAVA::Entity* e) {
                return Selectable(DAVA::Any(e));
            });

            SelectableGroup entitiesToMove;
            entitiesToMove.Add(objects);
            scene->structureSystem->Move(entitiesToMove, newParentEntity, insertBefore);
        }
    }

    // extract emitters for move
    {
        DAVA::ParticleEffectComponent* targetEffectComponent = DAVA::GetEffectComponent(newParentEntity);
        DAVA::Vector<DAVA::ParticleEmitterInstance*> emitters = mimeData->GetObjects<DAVA::ParticleEmitterInstance>();
        if (targetEffectComponent != nullptr && emitters.empty() == false)
        {
            DAVA::Vector<DAVA::ParticleEffectComponent*> oldComponents;
            oldComponents.reserve(emitters.size());

            int dropEmittersAfter = 0;
            if (positionInParent < static_cast<DAVA::int32>(targetEffectComponent->GetEmittersCount()))
            {
                dropEmittersAfter = positionInParent;
            }

            std::transform(emitters.begin(), emitters.end(), std::back_inserter(oldComponents),
                           [](const DAVA::ParticleEmitterInstance* emitter) {
                               DAVA::ParticleEffectComponent* component = emitter->GetOwner();
                               DVASSERT(component != nullptr);
                               return component;
                           });

            scene->structureSystem->MoveEmitter(emitters, oldComponents, targetEffectComponent, dropEmittersAfter);
        }
    }

    return true;
}

bool EntityTraits::CanBeEntityReparent(DAVA::Entity* entity, DAVA::Entity* parentCandidate) const
{
    DVASSERT(IsEditorLocalEntity(entity) == false);
    DVASSERT(entity->GetLocked() == false);

    if (IsDragNDropAllow(entity) == false)
    {
        return false;
    }

    if (DAVA::GetWaypointComponent(entity) && entity->GetParent() != parentCandidate)
    {
        return false;
    }

    if (GetPathComponent(entity) && (GetPathComponent(parentCandidate) || GetWaypointComponent(parentCandidate)))
    {
        return false;
    }

    return true;
}

bool EntityTraits::IsDragNDropAllow(DAVA::Entity* entity, bool isRoot) const
{
    auto hasDisableReparentMeta = [](DAVA::Entity* e) {
        DAVA::Reflection ref = DAVA::Reflection::Create(DAVA::ReflectedObject(e));
        DAVA::Reflection componentsRef = ref.GetField(DAVA::Entity::componentFieldString);
        DVASSERT(componentsRef.IsValid());

        DAVA::Vector<DAVA::Reflection::Field> component = componentsRef.GetFields();
        for (const DAVA::Reflection::Field& f : component)
        {
            DAVA::Any value = f.ref.GetValue();
            if (DAVA::TArc::GetTypeMeta<REMeta::DisableEntityReparent>(value) != nullptr)
            {
                return true;
            }
        }

        return false;
    };

    if (hasDisableReparentMeta(entity) == true)
    {
        return false;
    }

    if (isRoot == true)
    {
        DAVA::Entity* parent = entity->GetParent();
        while (parent != nullptr)
        {
            if (hasDisableReparentMeta(parent) == true)
            {
                return false;
            }
            parent = parent->GetParent();
        }
    }

    for (DAVA::int32 i = 0; i < entity->GetChildrenCount(); ++i)
    {
        if (IsDragNDropAllow(entity->GetChild(i), false) == false)
        {
            return false;
        }
    }

    return true;
}

bool EntityTraits::IsEditorLocalEntity(DAVA::Entity* entity) const
{
    return entity->GetName().find("editor.") != DAVA::String::npos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ParticleEmitterInstanceTraits::ParticleEmitterInstanceTraits(DAVA::TArc::ContextAccessor* accessor)
    : BaseSceneTreeTraits(accessor)
{
}

QIcon ParticleEmitterInstanceTraits::GetIcon(const Selectable& object) const
{
    return DAVA::TArc::SharedIcon(":/QtIcons/emitter_particle.png");
}

QString ParticleEmitterInstanceTraits::GetName(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleEmitterInstance>());
    DAVA::ParticleEmitterInstance* emitter = object.Cast<DAVA::ParticleEmitterInstance>();
    return QString(emitter->GetEmitter()->name.c_str());
}

DAVA::int32 ParticleEmitterInstanceTraits::GetChildrenCount(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleEmitterInstance>());
    DAVA::ParticleEmitterInstance* emitter = object.Cast<DAVA::ParticleEmitterInstance>();

    return static_cast<DAVA::int32>(emitter->GetEmitter()->layers.size());
}

void ParticleEmitterInstanceTraits::BuildUnfetchedList(const Selectable& object,
                                                       const DAVA::Function<bool(const Selectable&)>& isFetchedFn,
                                                       DAVA::Vector<DAVA::int32>& unfetchedIndexes) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleEmitterInstance>());
    DAVA::ParticleEmitterInstance* emitter = object.Cast<DAVA::ParticleEmitterInstance>();

    DAVA::Vector<DAVA::ParticleLayer*>& layers = emitter->GetEmitter()->layers;
    for (size_t i = 0; i < layers.size(); ++i)
    {
        DAVA::ParticleLayer* layer = layers[i];
        if (isFetchedFn(Selectable(DAVA::Any(layer))) == false)
        {
            unfetchedIndexes.push_back(static_cast<DAVA::int32>(i));
        }
    }
}

void ParticleEmitterInstanceTraits::FetchMore(const Selectable& object,
                                              const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                                              const DAVA::Function<void(DAVA::int32,
                                                                        const Selectable&)>& fetchCallback) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleEmitterInstance>());
    DAVA::ParticleEmitterInstance* emitter = object.Cast<DAVA::ParticleEmitterInstance>();

    DAVA::Vector<DAVA::ParticleLayer*>& layers = emitter->GetEmitter()->layers;
    for (DAVA::int32 index : unfetchedIndexes)
    {
        DAVA::ParticleLayer* layer = layers[index];
        fetchCallback(index, Selectable(DAVA::Any(layer)));
    }
}

Qt::ItemFlags ParticleEmitterInstanceTraits::GetItemFlags(const Selectable& object, Qt::ItemFlags defaultFlags) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleEmitterInstance>());
    DAVA::ParticleEmitterInstance* emitter = object.Cast<DAVA::ParticleEmitterInstance>();

    Qt::ItemFlags flags = BaseSceneTreeTraits::GetItemFlags(object, defaultFlags);
    if (emitter->GetOwner() == nullptr)
    {
        flags ^= Qt::ItemIsDragEnabled;
    }

    return flags;
}

QVariant ParticleEmitterInstanceTraits::GetValue(const Selectable& object, int itemRole) const
{
    if (itemRole == Qt::BackgroundRole)
    {
        DVASSERT(object.CanBeCastedTo<DAVA::ParticleEmitterInstance>());
        DAVA::ParticleEmitterInstance* emitter = object.Cast<DAVA::ParticleEmitterInstance>();
        if (emitter->GetEmitter()->shortEffect == true)
        {
            return QVariant(QBrush(QColor(255, 0, 0, 20)));
        }
    }

    return QVariant();
}

bool ParticleEmitterInstanceTraits::CanBeDropped(const ReflectedMimeData* mimeData, Qt::DropAction action,
                                                 const Selectable& parent, DAVA::int32 positionInParent) const
{
    return mimeData->HasObjects<DAVA::ParticleLayer>();
}

bool ParticleEmitterInstanceTraits::Drop(const ReflectedMimeData* mimeData, Qt::DropAction action,
                                         const Selectable& parent, DAVA::int32 positionInParent, SceneEditor2* scene) const
{
    DVASSERT(parent.CanBeCastedTo<DAVA::ParticleEmitterInstance>());
    DAVA::ParticleEmitterInstance* targetEmitter = parent.Cast<DAVA::ParticleEmitterInstance>();

    DAVA::Vector<DAVA::ParticleLayer*> layers = mimeData->GetObjects<DAVA::ParticleLayer>();
    DAVA::Vector<DAVA::ParticleEmitterInstance*> emitters;
    emitters.reserve(layers.size());

    EditorParticlesSystem* system = scene->GetSystem<EditorParticlesSystem>();
    for (DAVA::ParticleLayer* layer : layers)
    {
        emitters.push_back(system->GetDirectEmitterLayerOwner(layer));
    }

    DAVA::ParticleLayer* beforeLayer = nullptr;
    if (positionInParent < static_cast<DAVA::int32>(targetEmitter->GetEmitter()->layers.size()))
    {
        beforeLayer = targetEmitter->GetEmitter()->layers[positionInParent];
    }

    scene->structureSystem->MoveLayer(layers, emitters, targetEmitter, beforeLayer);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ParticleLayerTraits::ParticleLayerTraits(DAVA::TArc::ContextAccessor* accessor)
    : BaseSceneTreeTraits(accessor)
{
}

QIcon ParticleLayerTraits::GetIcon(const Selectable& object) const
{
    return DAVA::TArc::SharedIcon(":/QtIcons/layer_particle.png");
}

QString ParticleLayerTraits::GetName(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
    DAVA::ParticleLayer* layer = object.Cast<DAVA::ParticleLayer>();
    return QString(layer->layerName.c_str());
}

DAVA::int32 ParticleLayerTraits::GetChildrenCount(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
    DAVA::ParticleLayer* layer = object.Cast<DAVA::ParticleLayer>();
    DAVA::int32 forcesCount = static_cast<DAVA::int32>(layer->GetSimplifiedParticleForces().size());
    forcesCount += static_cast<DAVA::int32>(layer->GetParticleForces().size());

    if (layer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
    {
        DVASSERT(layer->innerEmitter != nullptr);
        ++forcesCount;
    }

    return forcesCount;
}

void ParticleLayerTraits::BuildUnfetchedList(const Selectable& object,
                                             const DAVA::Function<bool(const Selectable&)>& isFetchedFn,
                                             DAVA::Vector<DAVA::int32>& unfetchedIndexes) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
    DAVA::ParticleLayer* layer = object.Cast<DAVA::ParticleLayer>();
    DAVA::int32 fullCounter = 0;
    {
        const DAVA::Vector<DAVA::ParticleForceSimplified*>& forces = layer->GetSimplifiedParticleForces();
        for (DAVA::ParticleForceSimplified* force : forces)
        {
            if (isFetchedFn(Selectable(DAVA::Any(force))) == false)
            {
                unfetchedIndexes.push_back(fullCounter);
            }
            fullCounter++;
        }
    }

    {
        const DAVA::Vector<DAVA::ParticleForce*>& forces = layer->GetParticleForces();
        for (DAVA::ParticleForce* force : forces)
        {
            if (isFetchedFn(Selectable(DAVA::Any(force))) == false)
            {
                unfetchedIndexes.push_back(fullCounter);
            }
            fullCounter++;
        }
    }

    if (layer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
    {
        DVASSERT(layer->innerEmitter != nullptr);
        if (isFetchedFn(Selectable(DAVA::Any(layer->innerEmitter))) == false)
        {
            unfetchedIndexes.push_back(fullCounter);
        }
    }
}

void ParticleLayerTraits::FetchMore(const Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                                    const DAVA::Function<void(DAVA::int32, const Selectable&)>& fetchCallback) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
    DAVA::ParticleLayer* layer = object.Cast<DAVA::ParticleLayer>();
    const DAVA::Vector<DAVA::ParticleForceSimplified*>& simplifiedForces = layer->GetSimplifiedParticleForces();
    DAVA::int32 simplifiedCount = static_cast<DAVA::int32>(simplifiedForces.size());
    const DAVA::Vector<DAVA::ParticleForce*>& forces = layer->GetParticleForces();
    DAVA::int32 forcesCount = static_cast<DAVA::int32>(forces.size());
    for (DAVA::int32 index : unfetchedIndexes)
    {
        if (index < simplifiedCount)
        {
            DAVA::ParticleForceSimplified* force = simplifiedForces[index];
            fetchCallback(index, Selectable(DAVA::Any(force)));
        }
        else
        {
            index -= simplifiedCount;
            if (index < forcesCount)
            {
                DAVA::ParticleForce* force = forces[index];
                fetchCallback(index, Selectable(DAVA::Any(force)));
            }
            else
            {
                DVASSERT(index == forcesCount);
                DVASSERT(layer->type == DAVA::ParticleLayer::TYPE_SUPEREMITTER_PARTICLES);
                DVASSERT(layer->innerEmitter != nullptr);
                fetchCallback(forcesCount, Selectable(DAVA::Any(layer->innerEmitter)));
            }
        }
    }
}

Qt::ItemFlags ParticleLayerTraits::GetItemFlags(const Selectable& object, Qt::ItemFlags defaultFlags) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
    return BaseSceneTreeTraits::GetItemFlags(object, defaultFlags) | Qt::ItemIsUserCheckable;
}

QVariant ParticleLayerTraits::GetValue(const Selectable& object, int itemRole) const
{
    if (itemRole == Qt::CheckStateRole)
    {
        DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
        DAVA::ParticleLayer* layer = object.Cast<DAVA::ParticleLayer>();
        return QVariant(layer->isDisabled == true ? Qt::Unchecked : Qt::Checked);
    }
    else if (itemRole == Qt::ForegroundRole)
    {
        DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
        DAVA::ParticleLayer* layer = object.Cast<DAVA::ParticleLayer>();
    }

    return QVariant();
}

bool ParticleLayerTraits::SetValue(const Selectable& object, const QVariant& value, int itemRole,
                                   SceneEditor2* scene) const
{
    if (itemRole == Qt::CheckStateRole)
    {
        DVASSERT(object.CanBeCastedTo<DAVA::ParticleLayer>());
        DAVA::ParticleLayer* layer = object.Cast<DAVA::ParticleLayer>();
        Qt::CheckState state = value.value<Qt::CheckState>();
        DVASSERT(state != Qt::PartiallyChecked);
        bool isChecked = state == Qt::Checked ? true : false;
        scene->Exec(std::unique_ptr<DAVA::Command>(new CommandUpdateParticleLayerEnabled(layer, value.toBool())));
        return true;
    }

    return false;
}

bool ParticleLayerTraits::CanBeDropped(const ReflectedMimeData* mimeData, Qt::DropAction action,
                                       const Selectable& parent, DAVA::int32 positionInParent) const
{
    return mimeData->HasObjects<DAVA::ParticleForce>() || mimeData->HasObjects<DAVA::ParticleForceSimplified>();
}

bool ParticleLayerTraits::Drop(const ReflectedMimeData* mimeData, Qt::DropAction action,
                               const Selectable& parent, DAVA::int32 positionInParent, SceneEditor2* scene) const
{
    DVASSERT(parent.CanBeCastedTo<DAVA::ParticleLayer>());
    DAVA::ParticleLayer* targetLayer = parent.Cast<DAVA::ParticleLayer>();

    if (mimeData->HasObjects<DAVA::ParticleForce>())
    {
        DAVA::Vector<DAVA::ParticleForce*> forces = mimeData->GetObjects<DAVA::ParticleForce>();
        DVASSERT(forces.empty() == false);
        DAVA::Vector<DAVA::ParticleLayer*> layers;
        layers.reserve(forces.size());

        EditorParticlesSystem* system = scene->GetSystem<EditorParticlesSystem>();
        for (DAVA::ParticleForce* force : forces)
        {
            layers.push_back(system->GetForceOwner(force));
        }

        scene->structureSystem->MoveParticleForce(forces, layers, targetLayer);
    }
    else if (mimeData->HasObjects<DAVA::ParticleForceSimplified>())
    {
        DAVA::Vector<DAVA::ParticleForceSimplified*> forces = mimeData->GetObjects<DAVA::ParticleForceSimplified>();
        DVASSERT(forces.empty() == false);
        DAVA::Vector<DAVA::ParticleLayer*> layers;
        layers.reserve(forces.size());

        EditorParticlesSystem* system = scene->GetSystem<EditorParticlesSystem>();
        for (DAVA::ParticleForceSimplified* force : forces)
        {
            layers.push_back(system->GetForceOwner(force));
        }

        scene->structureSystem->MoveSimplifiedForce(forces, layers, targetLayer);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ParticleSimplifiedForceTraits::ParticleSimplifiedForceTraits(DAVA::TArc::ContextAccessor* accessor)
    : BaseSceneTreeTraits(accessor)
{
}

QIcon ParticleSimplifiedForceTraits::GetIcon(const Selectable& object) const
{
    return DAVA::TArc::SharedIcon(":/QtIcons/force.png");
}

QString ParticleSimplifiedForceTraits::GetName(const Selectable& object) const
{
    return QStringLiteral("force");
}

DAVA::int32 ParticleSimplifiedForceTraits::GetChildrenCount(const Selectable& object) const
{
    return 0;
}

Qt::ItemFlags ParticleSimplifiedForceTraits::GetItemFlags(const Selectable& object, Qt::ItemFlags defaultFlags) const
{
    return BaseSceneTreeTraits::GetItemFlags(object, defaultFlags) ^ Qt::ItemIsDropEnabled;
}

void ParticleSimplifiedForceTraits::BuildUnfetchedList(const Selectable&, const DAVA::Function<bool(const Selectable&)>&,
                                                       DAVA::Vector<DAVA::int32>&) const
{
}

void ParticleSimplifiedForceTraits::FetchMore(const Selectable&, const DAVA::Vector<DAVA::int32>&,
                                              const DAVA::Function<void(DAVA::int32, const Selectable&)>&) const
{
}

bool ParticleSimplifiedForceTraits::CanBeDropped(const ReflectedMimeData*, Qt::DropAction, const Selectable&, DAVA::int32) const
{
    return false;
}

bool ParticleSimplifiedForceTraits::Drop(const ReflectedMimeData*, Qt::DropAction, const Selectable&, DAVA::int32, SceneEditor2*) const
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////

ParticleForceTraits::ParticleForceTraits(DAVA::TArc::ContextAccessor* accessor)
    : BaseSceneTreeTraits(accessor)
{
}

QIcon ParticleForceTraits::GetIcon(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleForce>());
    DAVA::ParticleForce* force = object.Cast<DAVA::ParticleForce>();

    if (force->type == DAVA::ParticleForce::eType::DRAG_FORCE)
    {
        return force->isActive ? DAVA::TArc::SharedIcon(":/QtIcons/turtle.png") : DAVA::TArc::SharedIcon(":/QtIcons/turtle_bnw.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::VORTEX)
    {
        return force->isActive ? DAVA::TArc::SharedIcon(":/QtIcons/vortex_ico.png") : DAVA::TArc::SharedIcon(":/QtIcons/vortex_ico_red.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::GRAVITY)
    {
        return force->isActive ? DAVA::TArc::SharedIcon(":/QtIcons/gravity.png") : DAVA::TArc::SharedIcon(":/QtIcons/gravity_red.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::WIND)
    {
        return force->isActive ? DAVA::TArc::SharedIcon(":/QtIcons/wind_p.png") : DAVA::TArc::SharedIcon(":/QtIcons/wind_p_red.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::POINT_GRAVITY)
    {
        return force->isActive ? DAVA::TArc::SharedIcon(":/QtIcons/pointGravity.png") : DAVA::TArc::SharedIcon(":/QtIcons/pointGravity_red.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::PLANE_COLLISION)
    {
        return force->isActive ? DAVA::TArc::SharedIcon(":/QtIcons/plane_coll.png") : DAVA::TArc::SharedIcon(":/QtIcons/plane_coll_red.png");
    }

    return DAVA::TArc::SharedIcon(":/QtIcons/turtle.png");
}

QString ParticleForceTraits::GetName(const Selectable& object) const
{
    DVASSERT(object.CanBeCastedTo<DAVA::ParticleForce>());
    return object.Cast<DAVA::ParticleForce>()->forceName.c_str();
}

DAVA::int32 ParticleForceTraits::GetChildrenCount(const Selectable& object) const
{
    return 0;
}

Qt::ItemFlags ParticleForceTraits::GetItemFlags(const Selectable& object, Qt::ItemFlags defaultFlags) const
{
    return BaseSceneTreeTraits::GetItemFlags(object, defaultFlags) ^ Qt::ItemIsDropEnabled;
}

void ParticleForceTraits::BuildUnfetchedList(const Selectable&, const DAVA::Function<bool(const Selectable&)>&,
                                             DAVA::Vector<DAVA::int32>&) const
{
}

void ParticleForceTraits::FetchMore(const Selectable&, const DAVA::Vector<DAVA::int32>&,
                                    const DAVA::Function<void(DAVA::int32, const Selectable&)>&) const
{
}

bool ParticleForceTraits::CanBeDropped(const ReflectedMimeData*, Qt::DropAction, const Selectable&, DAVA::int32) const
{
    return false;
}

bool ParticleForceTraits::Drop(const ReflectedMimeData*, Qt::DropAction, const Selectable&, DAVA::int32, SceneEditor2*) const
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////

SceneTreeTraitsManager::SceneTreeTraitsManager(DAVA::TArc::ContextAccessor* accessor)
    : BaseSceneTreeTraits(accessor)
{
    AddTraitsNode<DAVA::Entity, EntityTraits>(accessor);
    AddTraitsNode<SceneEditor2, EntityTraits>(accessor);
    AddTraitsNode<DAVA::ParticleEmitterInstance, ParticleEmitterInstanceTraits>(accessor);
    AddTraitsNode<DAVA::ParticleLayer, ParticleLayerTraits>(accessor);
    AddTraitsNode<DAVA::ParticleForceSimplified, ParticleSimplifiedForceTraits>(accessor);
    AddTraitsNode<DAVA::ParticleForce, ParticleForceTraits>(accessor);
    std::sort(traits.begin(), traits.end(), [](const TraitsNode& left, const TraitsNode& right) {
        return left.type < right.type;
    });
}

QIcon SceneTreeTraitsManager::GetIcon(const Selectable& object) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return QIcon();
    }

    return traits->GetIcon(object);
}

QString SceneTreeTraitsManager::GetName(const Selectable& object) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return QString();
    }

    return traits->GetName(object);
}

QString SceneTreeTraitsManager::GetTooltip(const Selectable& object) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return QString();
    }

    return traits->GetTooltip(object);
}

DAVA::int32 SceneTreeTraitsManager::GetChildrenCount(const Selectable& object) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return 0;
    }

    return traits->GetChildrenCount(object);
}

void SceneTreeTraitsManager::BuildUnfetchedList(const Selectable& object,
                                                const DAVA::Function<bool(const Selectable&)>& isFetchedFn,
                                                DAVA::Vector<DAVA::int32>& unfetchedIndexes) const
{
    if (fetchBlocked == true)
    {
        return;
    }

    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return;
    }

    return traits->BuildUnfetchedList(object, isFetchedFn, unfetchedIndexes);
}

void SceneTreeTraitsManager::FetchMore(const Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                                       const DAVA::Function<void(DAVA::int32, const Selectable&)>& fetchCallback) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return;
    }

    return traits->FetchMore(object, unfetchedIndexes, fetchCallback);
}

Qt::ItemFlags SceneTreeTraitsManager::GetItemFlags(const Selectable& object, Qt::ItemFlags defaultFlags) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return defaultFlags;
    }

    return traits->GetItemFlags(object, defaultFlags);
}

QVariant SceneTreeTraitsManager::GetValue(const Selectable& object, int itemRole) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return QVariant();
    }

    return traits->GetValue(object, itemRole);
}

bool SceneTreeTraitsManager::SetValue(const Selectable& object, const QVariant& value, int itemRole,
                                      SceneEditor2* scene) const
{
    const BaseSceneTreeTraits* traits = GetTraits(object);
    if (traits == nullptr)
    {
        return false;
    }

    return traits->SetValue(object, value, itemRole, scene);
}

template <typename TObject, typename TTraits>
void SceneTreeTraitsManager::AddTraitsNode(DAVA::TArc::ContextAccessor* accessor)
{
    TraitsNode node;
    node.type = DAVA::ReflectedTypeDB::Get<TObject>();
    node.traits = DAVA::Any(TTraits(accessor));
    traits.push_back(node);
}

const BaseSceneTreeTraits* SceneTreeTraitsManager::GetTraits(const Selectable& object) const
{
    const DAVA::ReflectedType* objType = object.GetObjectType();
    auto predicate = [](const TraitsNode& node, const DAVA::ReflectedType* type) {
        return node.type < type;
    };

    auto iter = std::lower_bound(traits.begin(), traits.end(), objType, predicate);

    if (iter == traits.end())
    {
        DVASSERT(false);
        return nullptr;
    }

    DVASSERT(iter->type == objType);
    return reinterpret_cast<const BaseSceneTreeTraits*>(iter->traits.GetData());
}

bool SceneTreeTraitsManager::CanBeDropped(const ReflectedMimeData* mimeData, Qt::DropAction action,
                                          const Selectable& parent, DAVA::int32 positionInParent) const
{
    const BaseSceneTreeTraits* traits = GetTraits(parent);
    if (traits == nullptr)
    {
        return false;
    }

    return traits->CanBeDropped(mimeData, action, parent, positionInParent);
}

bool SceneTreeTraitsManager::Drop(const ReflectedMimeData* mimeData, Qt::DropAction action,
                                  const Selectable& parent, DAVA::int32 positionInParent, SceneEditor2* scene) const
{
    const BaseSceneTreeTraits* traits = GetTraits(parent);
    if (traits == nullptr)
    {
        return false;
    }

    if (positionInParent == -1)
    {
        positionInParent = 0;
    }

    return traits->Drop(mimeData, action, parent, positionInParent, scene);
}
