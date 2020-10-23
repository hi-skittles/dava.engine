#include "Base/BaseMath.h"
#include "WindSystem.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/WindComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Scene3D/Scene.h"
#include "Utils/Random.h"
#include "Math/Math2D.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Render/Renderer.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

namespace DAVA
{
const static float32 WIND_PERIOD = 2 * PI;

WindSystem::WindInfo::WindInfo(WindComponent* c)
    :
    component(c)
{
    timeValue = static_cast<float32>(GetEngineContext()->random->RandFloat(1000.f));
}

WindSystem::WindSystem(Scene* scene)
    :
    SceneSystem(scene)
{
    RenderOptions* options = Renderer::GetOptions();
    options->AddObserver(this);
    HandleEvent(options);

    isVegetationAnimationEnabled = QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_VEGETATION_ANIMATION);

    for (int32 i = 0; i < WIND_TABLE_SIZE; i++)
    {
        float32 t = WIND_PERIOD * i / static_cast<float32>(WIND_TABLE_SIZE);
        windValuesTable[i] = (2.f + std::sin(t) * 0.7f + std::cos(t * 10) * 0.3f);
    }
}

WindSystem::~WindSystem()
{
    DVASSERT(winds.size() == 0);

    Renderer::GetOptions()->RemoveObserver(this);
}

void WindSystem::AddEntity(Entity* entity)
{
    WindComponent* wind = GetWindComponent(entity);
    winds.push_back(new WindInfo(wind));
}

void WindSystem::RemoveEntity(Entity* entity)
{
    int32 windsCount = static_cast<int32>(winds.size());
    for (int32 i = 0; i < windsCount; ++i)
    {
        WindInfo* info = winds[i];
        if (info->component->GetEntity() == entity)
        {
            SafeDelete(info);
            RemoveExchangingWithLast(winds, i);
            break;
        }
    }
}

void WindSystem::PrepareForRemove()
{
    for (WindInfo* info : winds)
    {
        SafeDelete(info);
    }

    winds.clear();
}

void WindSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_WIND_SYSTEM)

    if (!isAnimationEnabled || !isVegetationAnimationEnabled)
        return;

    int32 windCount = static_cast<int32>(winds.size());
    for (int32 i = 0; i < windCount; ++i)
    {
        winds[i]->timeValue += timeElapsed * winds[i]->component->GetWindSpeed();
    }
}

Vector3 WindSystem::GetWind(const Vector3& inPosition) const
{
    Vector3 ret;
    int32 windCount = static_cast<int32>(winds.size());
    for (int32 i = 0; i < windCount; ++i)
    {
        WindInfo* info = winds[i];
        if (info->component->GetInfluenceBBox().IsInside(inPosition))
        {
            ret += info->component->GetDirection() * info->component->GetWindForce() * GetWindValueFromTable(inPosition, info) * winds[i]->component->GetWindSpeed();
        }
    }

    return ret;
}

float32 WindSystem::GetWindValueFromTable(const Vector3& inPosition, const WindInfo* info) const
{
    Vector3 dir = info->component->GetDirection();
    Vector3 projPt = dir * (inPosition.DotProduct(dir));
    float32 t = projPt.Length() + info->timeValue;

    float32 tMod = std::fmod(t, WIND_PERIOD);
    int32 i = static_cast<int32>(std::floor(tMod / WIND_PERIOD * WIND_TABLE_SIZE));

    DVASSERT(i >= 0 && i < WIND_TABLE_SIZE);
    return windValuesTable[i];
}

void WindSystem::HandleEvent(Observable* observable)
{
    RenderOptions* options = static_cast<RenderOptions*>(observable);
    isAnimationEnabled = options->IsOptionEnabled(RenderOptions::SPEEDTREE_ANIMATIONS);
}
};