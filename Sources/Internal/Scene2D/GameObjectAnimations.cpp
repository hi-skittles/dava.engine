#include "Scene2D/GameObject.h"
#include "Animation/AnimationManager.h"
#include "Scene2D/GameObjectManager.h"
#include "Scene2D/GameObjectAnimations.h"

namespace DAVA
{
RemoveFromManagerGameObjectAnimation::RemoveFromManagerGameObjectAnimation(GameObject* _object)
    : Animation(_object, 0.0f, Interpolation::LINEAR)
{
    object = _object;
}

RemoveFromManagerGameObjectAnimation::~RemoveFromManagerGameObjectAnimation()
{
}

void RemoveFromManagerGameObjectAnimation::OnStart()
{
    state |= STATE_FINISHED;
    GameObjectManager* man = object->GetManager();
    if (man)
        man->RemoveObject(object);
}

void RemoveFromManagerGameObjectAnimation::Update(float32 timeElapsed)
{
}
};