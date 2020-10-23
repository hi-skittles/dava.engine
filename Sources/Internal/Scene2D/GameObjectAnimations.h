#ifndef __DAVAENGINE_GAME_OBJECT_ANIMATIONS_H__
#define __DAVAENGINE_GAME_OBJECT_ANIMATIONS_H__

#include "Base/BaseMath.h"
#include "Render/2D/Sprite.h"
#include "Animation/AnimatedObject.h"
#include "Animation/Interpolation.h"

namespace DAVA
{
class RemoveFromManagerGameObjectAnimation : public Animation
{
protected:
    ~RemoveFromManagerGameObjectAnimation();

public:
    RemoveFromManagerGameObjectAnimation(GameObject* object);

    virtual void Update(float32 timeElapsed);
    virtual void OnStart();

private:
    GameObject* object;
};
};

#endif