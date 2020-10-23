#ifndef __DAVAENGINE_SCENE3D_WAVESYSTEM_H__
#define __DAVAENGINE_SCENE3D_WAVESYSTEM_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/Observer.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class Entity;
class WaveComponent;
class WaveSystem : public SceneSystem, public Observer
{
    struct WaveInfo
    {
        WaveInfo(WaveComponent* component);

        WaveComponent* component;
        float32 maxRadius;
        float32 maxRadiusSq;
        Vector3 center;
        float32 currentWaveRadius;
    };

public:
    WaveSystem(Scene* scene);
    virtual ~WaveSystem();

    void PrepareForRemove() override;
    void ImmediateEvent(Component* component, uint32 event) override;
    void Process(float32 timeElapsed) override;

    Vector3 GetWaveDisturbance(const Vector3& inPosition) const;

    void HandleEvent(Observable* observable) override;

protected:
    void ClearWaves();

    bool isWavesEnabled;
    bool isVegetationAnimationEnabled;

    Vector<WaveInfo*> waves;

    friend class WaveComponent;
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_WINDSYSTEM_H__ */
