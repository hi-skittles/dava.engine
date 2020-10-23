#ifndef __DAVAENGINE_SCENE3D_WINDSYSTEM_H__
#define __DAVAENGINE_SCENE3D_WINDSYSTEM_H__

#include "Base/BaseTypes.h"
#include "Base/Observer.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{

#define WIND_TABLE_SIZE 63

class Entity;
class WindComponent;
class WindSystem : public SceneSystem, public Observer
{
    struct WindInfo
    {
        WindInfo(WindComponent* c);

        WindComponent* component;
        float32 timeValue;
    };

public:
    WindSystem(Scene* scene);
    ~WindSystem() override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;
    void Process(float32 timeElapsed) override;

    Vector3 GetWind(const Vector3& inPosition) const;

    void HandleEvent(Observable* observable) override;

protected:
    float32 GetWindValueFromTable(const Vector3& inPosition, const WindInfo* info) const;

    Vector<WindInfo*> winds;
    bool isAnimationEnabled;
    bool isVegetationAnimationEnabled;

    float32 windValuesTable[WIND_TABLE_SIZE];
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_WINDSYSTEM_H__ */
