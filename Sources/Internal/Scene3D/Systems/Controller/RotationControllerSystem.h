#pragma once

#include "Entity/SceneSystem.h"
#include "Math/Vector.h"

namespace DAVA
{
class Camera;
class UIEvent;
class RotationControllerSystem : public SceneSystem
{
    static const float32 maxViewAngle;

public:
    RotationControllerSystem(Scene* scene);
    ~RotationControllerSystem() override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void Process(float32 timeElapsed) override;

    bool Input(UIEvent* event) override;

    float32 GetRotationSpeeed() const;
    void SetRotationSpeeed(float32 rotateSpeed);

    const Vector3& GetRotationPoint() const;
    void SetRotationPoint(const Vector3& point);

    void RecalcCameraViewAngles(Camera* camera);

private:
    void RotateDirection(Camera* camera);
    void RotatePosition(Camera* camera);
    void RotatePositionAroundPoint(Camera* camera, const Vector3& pos);

    Vector3 rotationPoint;

    Vector2 rotateStartPoint;
    Vector2 rotateStopPoint;

    float32 curViewAngleZ;
    float32 curViewAngleY;

    float32 rotationSpeed;

    Vector<Entity*> entities;

    Camera* oldCamera;
};

inline float32 RotationControllerSystem::GetRotationSpeeed() const
{
    return rotationSpeed;
}

inline void RotationControllerSystem::SetRotationSpeeed(float32 rotateSpeed)
{
    rotationSpeed = rotateSpeed;
}

inline const Vector3& RotationControllerSystem::GetRotationPoint() const
{
    return rotationPoint;
}
inline void RotationControllerSystem::SetRotationPoint(const Vector3& point)
{
    rotationPoint = point;
}
};
