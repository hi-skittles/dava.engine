#include "WASDControllerSystem.h"

#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/Controller/WASDControllerComponent.h"

#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"

#include "Render/Highlevel/Camera.h"

#include "Input/InputSystem.h"
#include "Input/Keyboard.h"

#include "Utils/Utils.h"

#include "DeviceManager/DeviceManager.h"

namespace DAVA
{
WASDControllerSystem::WASDControllerSystem(Scene* scene)
    : SceneSystem(scene)
    , moveSpeed(1.f)
{
}

WASDControllerSystem::~WASDControllerSystem()
{
}

void WASDControllerSystem::AddEntity(Entity* entity)
{
    DVASSERT(GetCamera(entity) != NULL && "Right now system works with camera only");

    entities.push_back(entity);
}

void WASDControllerSystem::RemoveEntity(Entity* entity)
{
    const bool removeResult = FindAndRemoveExchangingWithLast(entities, entity);
    DVASSERT(removeResult);
}

void WASDControllerSystem::PrepareForRemove()
{
    entities.clear();
}

void WASDControllerSystem::Process(float32 timeElapsed)
{
    float32 actualMoveSpeed = moveSpeed * timeElapsed;

    const uint32 size = static_cast<uint32>(entities.size());
    if (0 == size)
        return;

    Keyboard* keyboard = GetEngineContext()->deviceManager->GetKeyboard();
    if (keyboard == nullptr)
    {
        return;
    }

    if (keyboard->GetKeyState(eInputElements::KB_LSHIFT).IsPressed() ||
        keyboard->GetKeyState(eInputElements::KB_RCTRL).IsPressed() ||
        keyboard->GetKeyState(eInputElements::KB_LCTRL).IsPressed() ||
        keyboard->GetKeyState(eInputElements::KB_RALT).IsPressed() ||
        keyboard->GetKeyState(eInputElements::KB_LALT).IsPressed())
    {
        return;
    }

    for (uint32 i = 0; i < size; ++i)
    {
        Camera* camera = GetCamera(entities[i]);
        if ((camera != nullptr) && (camera == GetScene()->GetDrawCamera()))
        {
            if (keyboard->GetKeyState(eInputElements::KB_W).IsPressed() || keyboard->GetKeyState(eInputElements::KB_UP).IsPressed())
            {
                MoveForward(camera, actualMoveSpeed, DIRECTION_STRAIGHT);
            }
            if (keyboard->GetKeyState(eInputElements::KB_S).IsPressed() || keyboard->GetKeyState(eInputElements::KB_DOWN).IsPressed())
            {
                MoveForward(camera, actualMoveSpeed, DIRECTION_INVERSE);
            }
            if (keyboard->GetKeyState(eInputElements::KB_D).IsPressed() || keyboard->GetKeyState(eInputElements::KB_RIGHT).IsPressed())
            {
                MoveRight(camera, actualMoveSpeed, DIRECTION_STRAIGHT);
            }
            if (keyboard->GetKeyState(eInputElements::KB_A).IsPressed() || keyboard->GetKeyState(eInputElements::KB_LEFT).IsPressed())
            {
                MoveRight(camera, actualMoveSpeed, DIRECTION_INVERSE);
            }
        }
    }
}

void WASDControllerSystem::MoveForward(Camera* camera, float32 speed, eDirection direction)
{
    Vector3 pos = camera->GetPosition();
    const Vector3& dir = camera->GetDirection();

    pos += (dir * speed * static_cast<float32>(direction));

    camera->SetPosition(pos);
    camera->SetDirection(dir);
}

void WASDControllerSystem::MoveRight(Camera* camera, float32 speed, eDirection direction)
{
    Vector3 pos = camera->GetPosition();
    Vector3 right = -camera->GetLeft();
    const Vector3& dir = camera->GetDirection();

    pos += (right * speed * static_cast<float32>(direction));

    camera->SetPosition(pos);
    camera->SetDirection(dir);
}
};