#include "REPlatform/Global/SceneTree/SceneTreeFiltration.h"
#include "REPlatform/DataNodes/SelectionData.h"
#include "REPlatform/DataNodes/Selectable.h"

#include <TArc/Core/ContextAccessor.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/DataContext.h>

#include <Entity/Component.h>
#include <Particles/ParticleEmitterInstance.h>
#include <Particles/ParticleForce.h>
#include <Particles/ParticleForceSimplified.h>
#include <Particles/ParticleLayer.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Components/ActionComponent.h>
#include <Scene3D/Components/CameraComponent.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/LightComponent.h>
#include <Scene3D/Components/SkeletonComponent.h>
#include <Scene3D/Components/SoundComponent.h>
#include <Scene3D/Components/StaticOcclusionComponent.h>
#include <Scene3D/Components/SwitchComponent.h>
#include <Scene3D/Components/ParticleEffectComponent.h>
#include <Scene3D/Components/QualitySettingsComponent.h>
#include <Scene3D/Components/UserComponent.h>
#include <Scene3D/Components/VisibilityCheckComponent.h>
#include <Scene3D/Components/WaveComponent.h>
#include <Scene3D/Components/WindComponent.h>
#include <Scene3D/Components/Waypoint/PathComponent.h>
#include <Scene3D/Lod/LodComponent.h>

namespace DAVA
{
bool SceneTreeFilterBase::IsEnabled() const
{
    return isEnabled;
}

void SceneTreeFilterBase::SetEnabled(bool isEnabled_)
{
    if (isEnabled != isEnabled_)
    {
        isEnabled = isEnabled_;
        changed.Emit();
    }
}

bool SceneTreeFilterBase::IsInverted() const
{
    return isInverted;
}

void SceneTreeFilterBase::SetInverted(bool isInverted_)
{
    if (isInverted != isInverted_)
    {
        isInverted = isInverted_;
        changed.Emit();
    }
}

const char* SceneTreeFilterBase::titleFieldName = "title";
const char* SceneTreeFilterBase::enabledFieldName = "enabled";
const char* SceneTreeFilterBase::inversedFieldName = "inverted";

DAVA_VIRTUAL_REFLECTION_IMPL(SceneTreeFilterBase)
{
    ReflectionRegistrator<SceneTreeFilterBase>::Begin()
    .Field(titleFieldName, &SceneTreeFilterBase::GetTitle, nullptr)
    .Field(enabledFieldName, &SceneTreeFilterBase::IsEnabled, &SceneTreeFilterBase::SetEnabled)
    .Field(inversedFieldName, &SceneTreeFilterBase::IsInverted, &SceneTreeFilterBase::SetInverted)
    .End();
}

template <typename ComponentType>
class ComponentFilter : public SceneTreeFilterBase
{
public:
    ComponentFilter()
    {
        const ReflectedType* type = ReflectedTypeDB::GetByPointer(this);
        const M::DisplayName* displayName = GetReflectedTypeMeta<M::DisplayName>(type);
        if (displayName != nullptr)
        {
            title = QString::fromStdString(displayName->displayName);
        }
        else
        {
            title = QString::fromStdString(type->GetPermanentName());
        }
    }

    QString GetTitle() const override
    {
        return title;
    }

    bool IsMatched(const Selectable& object, ContextAccessor* /*accessor*/) const override
    {
        if (object.CanBeCastedTo<Entity>() == false)
        {
            return false;
        }

        Entity* entity = object.Cast<Entity>();
        return HasComponent(entity, Type::Instance<ComponentType>());
    }

private:
    QString title;

    DAVA_VIRTUAL_REFLECTION(ComponentFilter, SceneTreeFilterBase);
};

using LightComponentFilter = ComponentFilter<LightComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(LightComponentFilter)
{
    ReflectionRegistrator<LightComponentFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Light")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using CameraComponentFilter = ComponentFilter<CameraComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(CameraComponentFilter)
{
    ReflectionRegistrator<CameraComponentFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Camera")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using UserNodeComponentFilter = ComponentFilter<UserComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(UserNodeComponentFilter)
{
    ReflectionRegistrator<UserNodeComponentFilter>::Begin()[M::Group("Object Type"), M::DisplayName("User Node")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using SwitchNodeComponentFilter = ComponentFilter<SwitchComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(SwitchNodeComponentFilter)
{
    ReflectionRegistrator<SwitchNodeComponentFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Switch")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using ParticleEffectFilter = ComponentFilter<ParticleEffectComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(ParticleEffectFilter)
{
    ReflectionRegistrator<ParticleEffectFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Particle Effect Node")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using WindFilter = ComponentFilter<WindComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(WindFilter)
{
    ReflectionRegistrator<WindFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Wind")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using PathFilter = ComponentFilter<PathComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(PathFilter)
{
    ReflectionRegistrator<PathFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Path")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

class ParticleEmitterFilter : public SceneTreeFilterBase
{
public:
    bool IsMatched(const Selectable& object, ContextAccessor* accessor) const override
    {
        return object.CanBeCastedTo<ParticleEmitterInstance>();
    }

    QString GetTitle() const override
    {
        return QStringLiteral("Particle Emitter");
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ParticleEmitterFilter, SceneTreeFilterBase)
    {
        ReflectionRegistrator<ParticleEmitterFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Particle Emitter")]
        .ConstructorByPointer()
        .DestructorByPointer()
        .End();
    }
};

class ParticleLayerFilter : public SceneTreeFilterBase
{
public:
    bool IsMatched(const Selectable& object, ContextAccessor* accessor) const override
    {
        return object.CanBeCastedTo<ParticleLayer>();
    }

    QString GetTitle() const override
    {
        return QStringLiteral("Particle Layer");
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ParticleLayerFilter, SceneTreeFilterBase)
    {
        ReflectionRegistrator<ParticleLayerFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Particle Layer")]
        .ConstructorByPointer()
        .DestructorByPointer()
        .End();
    }
};

class ParticleForceFilter : public SceneTreeFilterBase
{
public:
    bool IsMatched(const Selectable& object, ContextAccessor* accessor) const override
    {
        return object.CanBeCastedTo<ParticleForce>() || object.CanBeCastedTo<ParticleForceSimplified>();
    }

    QString GetTitle() const override
    {
        return QStringLiteral("Particle Force");
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ParticleForceFilter, SceneTreeFilterBase)
    {
        ReflectionRegistrator<ParticleForceFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Particle Force")]
        .ConstructorByPointer()
        .DestructorByPointer()
        .End();
    }
};

class LandscapeFilter : public SceneTreeFilterBase
{
public:
    bool IsMatched(const Selectable& object, ContextAccessor* accessor) const override
    {
        if (object.CanBeCastedTo<Entity>() == false)
        {
            return false;
        }

        Entity* entity = object.Cast<Entity>();
        RenderObject* ro = GetRenderObject(entity);
        if (ro == nullptr)
        {
            return false;
        }

        return ro->GetType() == RenderObject::TYPE_LANDSCAPE;
    }

    QString GetTitle() const override
    {
        return QStringLiteral("Landscape");
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(LandscapeFilter, SceneTreeFilterBase)
    {
        ReflectionRegistrator<LandscapeFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Landscape")]
        .ConstructorByPointer()
        .DestructorByPointer()
        .End();
    }
};

class VegetationFilter : public SceneTreeFilterBase
{
public:
    bool IsMatched(const Selectable& object, ContextAccessor* accessor) const override
    {
        if (object.CanBeCastedTo<Entity>() == false)
        {
            return false;
        }

        Entity* entity = object.Cast<Entity>();
        RenderObject* ro = GetRenderObject(entity);
        if (ro == nullptr)
        {
            return false;
        }

        return ro->GetType() == RenderObject::TYPE_VEGETATION;
    }

    QString GetTitle() const override
    {
        return QStringLiteral("Vegetation");
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(VegetationFilter, SceneTreeFilterBase)
    {
        ReflectionRegistrator<VegetationFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Vegetation")]
        .ConstructorByPointer()
        .DestructorByPointer()
        .End();
    }
};

class EditorSpriteFilter : public SceneTreeFilterBase
{
public:
    bool IsMatched(const Selectable& object, ContextAccessor* accessor) const override
    {
        if (object.CanBeCastedTo<Entity>() == false)
        {
            return false;
        }

        Entity* entity = object.Cast<Entity>();
        RenderObject* ro = GetRenderObject(entity);
        if (ro == nullptr)
        {
            return false;
        }

        return ro->GetType() == RenderObject::TYPE_SPRITE;
    }

    QString GetTitle() const override
    {
        return QStringLiteral("Editor Sprite");
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(EditorSpriteFilter, SceneTreeFilterBase)
    {
        ReflectionRegistrator<EditorSpriteFilter>::Begin()[M::Group("Object Type"), M::DisplayName("Editor Sprite")]
        .ConstructorByPointer()
        .DestructorByPointer()
        .End();
    }
};

using ActionFilter = ComponentFilter<ActionComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(ActionFilter)
{
    ReflectionRegistrator<ActionFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Action")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using QualitySettingsFilter = ComponentFilter<QualitySettingsComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(QualitySettingsFilter)
{
    ReflectionRegistrator<QualitySettingsFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Quality Settings")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using StaticOcclusionFilter = ComponentFilter<StaticOcclusionComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(StaticOcclusionFilter)
{
    ReflectionRegistrator<StaticOcclusionFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Static Occlusion")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using SoundFilter = ComponentFilter<SoundComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(SoundFilter)
{
    ReflectionRegistrator<SoundFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Sound")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using WaveFilter = ComponentFilter<WaveComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(WaveFilter)
{
    ReflectionRegistrator<WaveFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Wave")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using SkeletonFilter = ComponentFilter<SkeletonComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(SkeletonFilter)
{
    ReflectionRegistrator<SkeletonFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Skeleton")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using VisibilityFilter = ComponentFilter<VisibilityCheckComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(VisibilityFilter)
{
    ReflectionRegistrator<VisibilityFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Visibility")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

using LodFilter = ComponentFilter<LodComponent>;
DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(LodFilter)
{
    ReflectionRegistrator<LodFilter>::Begin()[M::Group("Component Filter"), M::DisplayName("Lod")]
    .ConstructorByPointer()
    .DestructorByPointer()
    .End();
}

class InSelectionFilter : public SceneTreeFilterBase
{
public:
    bool IsMatched(const Selectable& object, ContextAccessor* accessor) const override
    {
        DataContext* ctx = accessor->GetActiveContext();

        SelectionData* data = ctx->GetData<SelectionData>();
        return data->GetSelection().ContainsObject(object.GetContainedObject());
    }

    QString GetTitle() const override
    {
        if (IsInverted() == false)
            return QStringLiteral("In selection");

        return QStringLiteral("Not in selection");
    }

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(InSelectionFilter, SceneTreeFilterBase)
    {
        ReflectionRegistrator<InSelectionFilter>::Begin()[M::DisplayName("In selection")]
        .ConstructorByPointer()
        .DestructorByPointer()
        .End();
    }
};

void RegisterPredefinedFilters()
{
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(LightComponentFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(CameraComponentFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(UserNodeComponentFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(SwitchNodeComponentFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(ParticleEffectFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(ParticleEmitterFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(ParticleLayerFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(ParticleForceFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(LandscapeFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(WindFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(VegetationFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(PathFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(EditorSpriteFilter);

    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(ActionFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(QualitySettingsFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(StaticOcclusionFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(SoundFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(WaveFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(SkeletonFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(VisibilityFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(LodFilter);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(InSelectionFilter);
}
} // namespace DAVA
