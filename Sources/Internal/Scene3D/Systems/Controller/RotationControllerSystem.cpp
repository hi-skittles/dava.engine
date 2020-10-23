#include "RotationControllerSystem.h"

#include "Scene3D/Components/Controller/RotationControllerComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"

#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"

#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

#include "Render/Highlevel/Camera.h"

#include "Input/InputSystem.h"
#include "Input/Keyboard.h"

#include "UI/UIEvent.h"

#include "DeviceManager/DeviceManager.h"

namespace DAVA
{
const float32 RotationControllerSystem::maxViewAngle = 89.0f;

RotationControllerSystem::RotationControllerSystem(Scene* scene)
    : SceneSystem(scene)
    , curViewAngleZ(0)
    , curViewAngleY(0)
    , rotationSpeed(0.15f)
    , oldCamera(NULL)
{
}

RotationControllerSystem::~RotationControllerSystem() = default;

void RotationControllerSystem::AddEntity(Entity* entity)
{
    DVASSERT(GetCamera(entity) != NULL && "Right now system works with camera only");

    entities.push_back(entity);
}

void RotationControllerSystem::RemoveEntity(Entity* entity)
{
    const bool removeResult = FindAndRemoveExchangingWithLast(entities, entity);
    DVASSERT(removeResult);
}

void RotationControllerSystem::PrepareForRemove()
{
    entities.clear();
}

void RotationControllerSystem::Process(float32 timeElapsed)
{
    Camera* camera = GetScene()->GetDrawCamera();
    if (camera != oldCamera)
    {
        oldCamera = camera;
        RecalcCameraViewAngles(camera);
    }
}

bool RotationControllerSystem::Input(UIEvent* event)
{
    const uint32 size = static_cast<uint32>(entities.size());
    if (0 == size)
    {
        return false;
    }

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    if (event->mouseButton == eMouseButtons::RIGHT || event->mouseButton == eMouseButtons::MIDDLE)
#endif
    {
        if (UIEvent::Phase::BEGAN == event->phase)
        {
            rotateStartPoint = event->point;
            rotateStopPoint = event->point;
        }
        else if (UIEvent::Phase::DRAG == event->phase || UIEvent::Phase::ENDED == event->phase)
        {
            rotateStartPoint = rotateStopPoint;
            rotateStopPoint = event->point;

            Camera* camera = GetScene()->GetDrawCamera();
            if (!camera)
            {
                return false;
            }

            //Find active wasd component
            for (uint32 i = 0; i < size; ++i)
            {
                if (GetCamera(entities[i]) == camera)
                {
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_WIN_UAP__)
                    RotateDirection(camera);
#else
                    if (event->mouseButton == eMouseButtons::RIGHT)
                    {
                        RotateDirection(camera);
                    }
                    else if (event->mouseButton == eMouseButtons::MIDDLE)
                    {
                        Keyboard* keyboard = GetEngineContext()->deviceManager->GetKeyboard();
                        if (keyboard != nullptr)
                        {
                            if (keyboard->GetKeyState(eInputElements::KB_LALT).IsPressed() || keyboard->GetKeyState(eInputElements::KB_RALT).IsPressed())
                            {
                                RotatePositionAroundPoint(camera, rotationPoint);
                            }
                            else
                            {
                                RotatePosition(camera);
                            }
                        }
                    }
#endif
                }
            }
        }
    }
    return false;
}

void RotationControllerSystem::RotateDirection(Camera* camera)
{
    if (!camera->GetIsOrtho())
    {
        DAVA::Vector2 dp = rotateStopPoint - rotateStartPoint;
        curViewAngleZ += dp.x * rotationSpeed;
        curViewAngleY = Clamp(curViewAngleY + dp.y * rotationSpeed, -maxViewAngle, maxViewAngle);

        DAVA::Matrix4 mt, mt2;
        mt.BuildRotation(DAVA::Vector3(0.f, 0.f, 1.f), -DAVA::DegToRad(curViewAngleZ));
        mt2.BuildRotation(DAVA::Vector3(1.f, 0.f, 0.f), -DAVA::DegToRad(curViewAngleY));
        mt2 *= mt;

        DAVA::Vector3 dir = DAVA::Vector3(0.f, 10.f, 0.f) * mt2;
        camera->SetDirection(dir);
    }
}

void RotationControllerSystem::RotatePosition(Camera* camera)
{
    DAVA::Vector2 dp = rotateStopPoint - rotateStartPoint;
    DAVA::Matrix4 mt, mt1, mt2, mt3;

    mt1.BuildTranslation(DAVA::Vector3(-dp.x * rotationSpeed, 0.f, dp.y * rotationSpeed));
    mt2.BuildRotation(DAVA::Vector3(1.f, 0.f, 0.f), -DAVA::DegToRad(curViewAngleY));
    mt3.BuildRotation(DAVA::Vector3(0.f, 0.f, 1.f), -DAVA::DegToRad(curViewAngleZ));

    mt = mt1 * mt2 * mt3;

    DAVA::Vector3 pos = camera->GetPosition() + (DAVA::Vector3(0, 0, 0) * mt);
    DAVA::Vector3 dir = camera->GetDirection();

    camera->SetPosition(pos);
    camera->SetDirection(dir);
}

void RotationControllerSystem::RotatePositionAroundPoint(Camera* camera, const Vector3& pos)
{
    curViewAngleZ += (rotateStopPoint.x - rotateStartPoint.x);
    curViewAngleY = Clamp(curViewAngleY + (rotateStopPoint.y - rotateStartPoint.y), -maxViewAngle, maxViewAngle);

    DAVA::Matrix4 mt, mt2;
    mt.BuildRotation(DAVA::Vector3(0, 0, 1), -DAVA::DegToRad(curViewAngleZ));
    mt2.BuildRotation(DAVA::Vector3(1, 0, 0), -DAVA::DegToRad(curViewAngleY));
    mt2 *= mt;

    DAVA::Vector3 curPos = camera->GetPosition();
    DAVA::float32 radius = (pos - curPos).Length();
    DAVA::Vector3 newPos = pos - DAVA::Vector3(0, radius, 0) * mt2;

    camera->SetPosition(newPos);
    camera->SetTarget(pos);
}

void RotationControllerSystem::RecalcCameraViewAngles(Camera* camera)
{
    if (NULL != camera)
    {
        DAVA::Vector3 dir = camera->GetDirection();
        DAVA::Vector2 dirXY(dir.x, dir.y);
        DAVA::Vector3 dirXY0(dir.x, dir.y, 0.0f);

        if (!dirXY.IsZero())
        {
            dirXY.Normalize();
            curViewAngleZ = -(DAVA::RadToDeg(dirXY.Angle()) - 90.0f);
        }
        else
        {
            curViewAngleZ = 0;
        }

        if (!dirXY0.IsZero())
        {
            dirXY0.Normalize();
            DAVA::float32 cosA = dirXY0.DotProduct(dir);
            curViewAngleY = DAVA::RadToDeg(acos(cosA));
        }
        else
        {
            curViewAngleY = 0;
        }

        if (curViewAngleY > maxViewAngle)
            curViewAngleY -= 360;

        if (curViewAngleY < -maxViewAngle)
            curViewAngleY += 360;
    }
    else
    {
        curViewAngleY = 0;
        curViewAngleZ = 0;
    }
}
};
