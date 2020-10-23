#include "WaveSystem.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/WaveComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Scene3D/Scene.h"
#include "Utils/Random.h"
#include "Math/Math2D.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Render/Renderer.h"

namespace DAVA
{
WaveSystem::WaveInfo::WaveInfo(WaveComponent* _component)
    : component(_component)
    , currentWaveRadius(0.f)
{
    if (component->GetDampingRatio() < EPSILON)
        maxRadius = component->GetInfluenceRadius();
    else
        maxRadius = Min(component->GetInfluenceRadius(), 1.f / component->GetDampingRatio());

    maxRadiusSq = maxRadius * maxRadius;

    center = GetTransformComponent(component->GetEntity())->GetWorldTransform().GetTranslation();
}

WaveSystem::WaveSystem(Scene* scene)
    :
    SceneSystem(scene)
{
    RenderOptions* options = Renderer::GetOptions();
    options->AddObserver(this);
    HandleEvent(options);

    isVegetationAnimationEnabled = QualitySettingsSystem::Instance()->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_VEGETATION_ANIMATION);

    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::WAVE_TRIGGERED);
}

WaveSystem::~WaveSystem()
{
    Renderer::GetOptions()->RemoveObserver(this);

    ClearWaves();
}

void WaveSystem::PrepareForRemove()
{
}

void WaveSystem::ImmediateEvent(Component* component, uint32 event)
{
    if (event == EventSystem::WAVE_TRIGGERED)
    {
        if (!isWavesEnabled || !isVegetationAnimationEnabled)
            return;

        WaveComponent* waveComponent = DynamicTypeCheck<WaveComponent*>(component);
        waves.push_back(new WaveInfo(waveComponent));
    }
}

void WaveSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_WAVE_SYSTEM);

    int32 index = 0;
    int32 size = static_cast<int32>(waves.size());
    while (index < size)
    {
        WaveInfo* info = waves[index];
        info->currentWaveRadius += info->component->GetWaveSpeed() * timeElapsed;

        if (info->currentWaveRadius >= info->maxRadius)
        {
            SafeDelete(info);
            RemoveExchangingWithLast(waves, index);
            size--;
        }
        else
        {
            index++;
        }
    }
}

Vector3 WaveSystem::GetWaveDisturbance(const Vector3& inPosition) const
{
    Vector3 ret;
    int32 wavesCount = static_cast<int32>(waves.size());
    for (int32 i = 0; i < wavesCount; ++i)
    {
        WaveInfo* info = waves[i];
        Vector3 direction = inPosition - info->center;
        float32 distanceSq = direction.SquareLength();
        if (distanceSq > EPSILON && distanceSq < info->maxRadiusSq)
        {
            WaveComponent* component = info->component;

            float32 damping = 1 - component->GetDampingRatio() * info->currentWaveRadius; //damping function: D = 1 - k * x

            DVASSERT(damping >= 0.f);

            float32 distance = std::sqrt(distanceSq);
            direction /= distance;
            float32 dt = Abs(info->currentWaveRadius - distance);
            float32 value = Max(1 - dt / component->GetWaveLenght(), 0.f) * component->GetWaveAmplitude() * component->GetWaveSpeed() * damping; // wave function: A = (1 - x/L) * A0

            DVASSERT(value >= 0.f);

            ret += direction * value;
        }
    }

    return ret;
}

void WaveSystem::ClearWaves()
{
    for (auto& wave : waves)
    {
        SafeDelete(wave);
    }

    waves.clear();
}

void WaveSystem::HandleEvent(Observable* observable)
{
    RenderOptions* options = static_cast<RenderOptions*>(observable);
    isWavesEnabled = options->IsOptionEnabled(RenderOptions::WAVE_DISTURBANCE_PROCESS);

    if (!isWavesEnabled)
        ClearWaves();
}
};