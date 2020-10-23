#include "Scene2D/GameObject.h"
#include "Animation/AnimationManager.h"
#include "Logger/Logger.h"
#include "Scene2D/GameObjectManager.h"

#include "Animation/LinearAnimation.h"
#include "Animation/BezierSplineAnimation.h"
#include "Collision/CollisionObject2.h"
#include "Animation/KeyframeAnimation.h"
#include "Scene2D/GameObjectAnimations.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/RenderHelper.h"

namespace DAVA
{
GameObject* GameObject::Create(const FilePath& _pathToSprite, int32 frame)
{
    GameObject* object = new GameObject();
    Sprite* sprite = Sprite::Create(_pathToSprite);
    object->SetSprite(sprite);
    object->SetFrame(frame);
    SafeRelease(sprite);

    return object;
}

GameObject* GameObject::Create(Sprite* sprite, int32 frame)
{
    GameObject* object = new GameObject();
    object->SetSprite(sprite);
    object->SetFrame(frame);
    return object;
}

GameObject::GameObject()
{
    //Logger::FrameworkDebug("[GameObject] ctor");
    //GetEngineContext()->animationManager->AddObject(this);
    visible = true;
    dead = false;
    priorityChanged = false;
    addedObject = false;
    sprite = 0;
    manager = 0;
    color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    blending = BLENDING_ALPHABLEND;

    collision = 0;
    groupId = 0;
    priority = 0;
    parent = 0;
    userData = 0;
    isDebugDraw = false;
    nextManager = 0;
}

GameObject::~GameObject()
{
    SafeRelease(collision);
    SafeRelease(sprite);
    //Logger::FrameworkDebug("[GameObject] destructor");
    //GetEngineContext()->animationManager->RemoveObject(this);
}

Animation* GameObject::WaitAnimation(float32 time, int32 track)
{
    Animation* animation = new Animation(this, time, Interpolation::LINEAR);
    animation->Start(track);
    return animation;
}

Animation* GameObject::MoveAnimation(float32 x, float32 y, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    LinearAnimation<Vector2>* animation = new LinearAnimation<Vector2>(this, &localDrawState.position, Vector2(x, y), time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* GameObject::MoveBySplineAnimation(BezierSpline2* spline, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    BezierSplineAnimation2* animation = new BezierSplineAnimation2(this, &localDrawState.position, spline, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* GameObject::ScaleAnimation(const Vector2& finalScale, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    LinearAnimation<Vector2>* animation = new LinearAnimation<Vector2>(this, &localDrawState.scale, finalScale, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* GameObject::ColorAnimation(const Color& finalColor, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    LinearAnimation<Color>* animation = new LinearAnimation<Color>(this, &color, finalColor, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

void GameObject::VisibleAnimationCallback(BaseObject* caller, void* param, void* callerData)
{
    bool* params = static_cast<bool*>(param);
    SetVisible(params[0]);
    delete[] params;
}

Animation* GameObject::VisibleAnimation(bool visible, int32 track /* = 0*/)
{
    //TODO: change to bool animation - Dizz
    Animation* animation = new Animation(this, 0.01f, Interpolation::LINEAR);
    bool* params = new bool[1];
    params[0] = visible;
    animation->AddEvent(Animation::EVENT_ANIMATION_START, Message(this, &GameObject::VisibleAnimationCallback, static_cast<void*>(params)));
    animation->Start(track);
    return animation;
}

Animation* GameObject::RotateAnimation(float32 newAngle, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    LinearAnimation<float32>* animation = new LinearAnimation<float32>(this, &localDrawState.angle, newAngle, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* GameObject::FrameInterpolateAnimation(int32 startFrame, int32 endFrame, float32 time, int32 track)
{
    KeyframeData* data = new KeyframeData();
    data->AddKeyframe(startFrame, 0.0f);
    data->AddKeyframe(endFrame, time);
    KeyframeAnimation* animation = new KeyframeAnimation(this, &localDrawState.frame, data, time);
    SafeRelease(data);
    animation->Start(track);
    return animation;
}

void GameObject::RemoveFromManagerAnimation(BaseObject* animation, void* userData, void* callerData)
{
    if (manager)
        manager->RemoveObject(this);
}

Animation* GameObject::RemoveFromManagerAnimation(int32 track)
{
    Animation* animation = new Animation(this, 0.001f, Interpolation::LINEAR);
    animation->AddEvent(Animation::EVENT_ANIMATION_START, Message(this, &GameObject::RemoveFromManagerAnimation));
    animation->Start(track);
    return animation;
}

/*
void GameObject::ApplyAnimation(SceneObjectAnimation * _animation)
{
	animations.push_back(_animation);
	visible = true; // make visible all objects if we started animation
}
	
void GameObject::ApplyAnimationWithDelay(SceneObjectAnimation * _animation, float32 _delayTime)
{
//	animation = _animation;
//	if (animation)
//	{
//		// set public properties of object
//		animation->position = position;
//		animation->angle = angle;
//		
//		// start animation
//		animation->StartAnimationWithDelay(_animationTime, _delayTime);
//	} 
}


void GameObject::Update(float32 timeElapsed)
{
	if ((!currentAnimationState) && (animations.size() > 0))
	{
		AnimationState * state = SceneManager::Instance()->NewAnimationState();
		state->startPosition = position;
		state->startAngle = angle;
		state->startFrame = frame;
		
		state->position = &position;
		state->angle = &angle;
		state->frame = &frame;
		state->animation = animations.front();
		state->animationTimeLength = state->animation->animationTimeLength;
		state->interpolationFunc = state->animation->func;
		animations.pop_front();
		
		currentAnimationState = state;
		
		// start animation
		SceneManager::Instance()->StartAnimationImmediatelly(state);
	}
	
	if (currentAnimationState)
		if (!currentAnimationState->isAnimating)
		{
			currentAnimationState->canRemove = true;
			currentAnimationState = 0;
		}
	
	if ((currentAnimationState == 0) && (animations.size() == 0) && (makeInvisibleAfterAllAnimations))
		visible = false;
} */

void GameObject::SetFrame(int32 _frame)
{
    localDrawState.frame = _frame;
    globalDrawState.frame = _frame;
}

void GameObject::SetPriority(int32 _priority)
{
    priority = _priority;
    if (manager)
    {
        manager->MarkToChangeObjectPriority(this);
    }
}

void GameObject::SetManager(GameObjectManager* _manager)
{
    manager = _manager;
}

void GameObject::FastForward(float32 skipTime, float32 updateTime)
{
    DVASSERT(skipTime > 0.0f && updateTime > 0.0f, "FastForward skipTime and updateTime must be greater than 0");
    float32 remainingSkipTime = skipTime;
    while (remainingSkipTime > 0.0f)
    {
        Update(updateTime);
        remainingSkipTime -= updateTime;
    }
}

void GameObject::Update(float32 timeElapsed)
{
    if (collision)
        collision->Update(globalDrawState);

    if (!children.empty())
    {
        List<GameObject*>::iterator it_end = children.end();
        for (List<GameObject*>::iterator t = children.begin(); t != it_end; ++t)
        {
            GameObject* o = *t;
            o->Update(timeElapsed);
        }
    }
}

void GameObject::RecalcHierarchy(const SpriteDrawState& parentDrawState)
{
    globalDrawState.BuildStateFromParentAndLocal(parentDrawState, localDrawState);
    if (!children.empty())
    {
        List<GameObject*>::iterator it_end = children.end();
        for (List<GameObject*>::iterator t = children.begin(); t != it_end; ++t)
        {
            GameObject* o = *t;
            o->RecalcHierarchy(globalDrawState);
        }
    }
}

void GameObject::CollisionPhase()
{
}

void GameObject::Draw()
{
    if (!visible)
        return;

    if (sprite)
    {
        RenderSystem2D::Instance()->Draw(sprite, &globalDrawState, color);

        //		RenderSystem2D::Instance()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
        //		RenderManager::Instance()->FillRect(Rect(globalDrawState.position.x - 1, globalDrawState.position.y - 1, 3, 3));
        //		RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        //		RenderSystem2D::Instance()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
        //		RenderManager::Instance()->FillRect(Rect(globalDrawState.position.x, globalDrawState.position.y, 10, 10));
        //		RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (isDebugDraw && collision)
    {
        collision->DebugDraw();
    }
    //	if (align == ALIGN_LEFTTOP)
    //	{
    //		//[sprite drawFrameF:frame atX:position.x atY:position.y];
    //	}else if (align == ALIGN_CENTER)
    //	{
    //		//[sprite drawFrameF:frame atCenterX:position.x atCenterY:position.y];
    //	}
}

void GameObject::AttachObject(GameObject* gameObject)
{
    children.push_back(gameObject);
    gameObject->parent = this;
}

void GameObject::DetachObject(GameObject* gameObject)
{
    for (List<GameObject*>::iterator t = children.begin(); t != children.end(); ++t)
    {
        if (*t == gameObject)
        {
            gameObject->localDrawState = gameObject->globalDrawState;
            gameObject->parent = 0;
            children.erase(t);
            break;
        }
    }
}

void GameObject::AddObject(GameObject* gameObject)
{
    children.push_back(gameObject);
    gameObject->parent = this;

    //gameObject->needToBeAddedToManager = true;
    //manager->changesStack.push_back(gameObject);

    if (manager)
        gameObject->ChangeManager(manager);
}

void GameObject::RemoveObject(GameObject* gameObject)
{
    //std::remove(animations.begin(), animations.end(), animation);

    for (List<GameObject*>::iterator t = children.begin(); t != children.end(); ++t)
    {
        if (*t == gameObject)
        {
            gameObject->parent = 0;
            gameObject->ChangeManager(0);
            children.erase(t);
            break;
        }
    }
}

GameObject* GameObject::GetParent() const
{
    return parent;
}

void GameObject::ChangeManager(GameObjectManager* newManager)
{
    Retain();
    if (manager)
    {
        manager->RemoveObject(this);
    }

    if (newManager)
    {
        newManager->AddObject(this);

        for (List<GameObject*>::iterator t = children.begin(); t != children.end(); ++t)
        {
            GameObject* child = *t;
            child->ChangeManager(newManager);
        }
    }

    Release();
}

void GameObject::SetPivotPoint(int32 alignFlags)
{ //WTF? Pivot point it's a pivot point. Align it's align. Why function that sets align calls SetPivotPoint!!!!????
    if (!sprite)
        return;

    if (alignFlags & ALIGN_LEFT)
    {
        localDrawState.pivotPoint.x = 0.0f;
    }
    else if (alignFlags & ALIGN_HCENTER)
    {
        localDrawState.pivotPoint.x = sprite->GetWidth() / 2.0f;
    }
    else if (alignFlags & ALIGN_RIGHT)
    {
        localDrawState.pivotPoint.x = sprite->GetWidth();
    }

    if (alignFlags & ALIGN_TOP)
    {
        localDrawState.pivotPoint.y = 0.0f;
    }
    else if (alignFlags & ALIGN_VCENTER)
    {
        localDrawState.pivotPoint.y = sprite->GetHeight() / 2.0f;
    }
    else if (alignFlags & ALIGN_BOTTOM)
    {
        localDrawState.pivotPoint.y = sprite->GetHeight();
    }
}

bool GameObject::IsCollideWith(GameObject* gameObject)
{
    CollisionObject2* collision2 = gameObject->GetCollision();
    if (collision && collision2)
    {
        collision->Update(globalDrawState);
        collision2->Update(gameObject->globalDrawState);
        return collision->IsCollideWith(collision2);
    }
    return false;
}

bool GameObject::IsCollideWith(CollisionObject2* collision2)
{
    if (collision && collision2)
    {
        collision->Update(globalDrawState);
        //collision2->Update(gameObject->globalDrawState);
        return collision->IsCollideWith(collision2);
    }
    return false;
}

void GameObject::SetCollisionObject(CollisionObject2* obj)
{
    SafeRelease(collision);
    collision = SafeRetain(obj);
}

List<GameObject*>& GameObject::GetChildren()
{
    return children;
}
};