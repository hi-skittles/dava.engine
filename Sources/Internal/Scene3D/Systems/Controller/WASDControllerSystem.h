#pragma once

#include "Entity/SceneSystem.h"

namespace DAVA
{
class Camera;
class UIEvent;
class InputCallback;

class WASDControllerSystem : public SceneSystem
{
    enum eDirection
    {
        DIRECTION_STRAIGHT = 1,
        DIRECTION_INVERSE = -1
    };

public:
    WASDControllerSystem(Scene* scene);
    virtual ~WASDControllerSystem();

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void Process(float32 timeElapsed) override;

    float32 GetMoveSpeeed() const;
    void SetMoveSpeed(float32 moveSpeed);

private:
    void MoveForward(Camera* camera, float32 speed, eDirection direction);
    void MoveRight(Camera* camera, float32 speed, eDirection direction);

    float32 moveSpeed;

    Vector<Entity*> entities;
};

inline float32 WASDControllerSystem::GetMoveSpeeed() const
{
    return moveSpeed;
}

inline void WASDControllerSystem::SetMoveSpeed(float32 _moveSpeed)
{
    moveSpeed = _moveSpeed;
}
};