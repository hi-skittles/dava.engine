#ifndef __DAVAENGINE_LINEAR_ANIMATION_H__
#define __DAVAENGINE_LINEAR_ANIMATION_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Animation/Animation.h"
#include "Animation/AnimatedObject.h"
#include "Math/Rect.h"

namespace DAVA
{
/**
	\ingroup animationsystem
	\brief Template class to make linear animations of objects
	This class is base class to create linear animations. It can be used to animation any values that 
	supports the following operators: [+, -, *]
	This class interpolate from currentValue of the variable and finish then value is reached some endValue. 
	This class is used inside all animation functions that implemented in UIControl & GameObject. 
	
	Usage example to interpolate float32 value: 
	\code
	LinearAnimation<float32> * animation = new LinearAnimation<float32>(this, &localDrawState.angle, newAngle, time, interpolationFunc);
	animation->Start(track);
	// Be very carefull here, it's only one case in our SDK where you do not need to call SafeRelease for object you've created
	// Releasing of all animations is handled by AnimationManager itself
	\endcode
	
	Usage example to interpolate Vector2 value: 
	\code
	LinearAnimation<float32> * animation = new LinearAnimation<Vector2>(this, &localDrawState.position, newPosition, time, interpolationFunc);
	animation->Start(track);
	// Be very carefull here, it's only one case in our SDK where you do not need to call SafeRelease for object you've created
	// Releasing of all animations is handled by AnimationManager itself
	\endcode
 */
template <class T>
class LinearAnimation : public Animation
{
protected:
    ~LinearAnimation()
    {
    }

public:
    LinearAnimation(AnimatedObject* _owner, T* _var, T _endValue, float32 _animationTimeLength, Interpolation::Func _iFunc);
    LinearAnimation(AnimatedObject* _owner, T* _var, T _endValue, float32 _animationTimeLength, Interpolation::FuncType _iType);
    // TODO:
    //LinearAnimation(AnimatedObject * _owner, T * _var, T _endValue, float32 _fixedSpeed);

    virtual void Update(float32 timeElapsed);
    virtual void OnStart();

protected:
    T* var;
    T endValue;
    T startValue;
};

template <class T>
LinearAnimation<T>::LinearAnimation(AnimatedObject* _owner, T* _var, T _endValue, float32 _animationTimeLength, Interpolation::FuncType _iType)
    : Animation(_owner, _animationTimeLength, _iType)
{
    var = _var;
    endValue = _endValue;
}

template <class T>
LinearAnimation<T>::LinearAnimation(AnimatedObject* _owner, T* _var, T _endValue, float32 _animationTimeLength, Interpolation::Func _iFunc)
    : Animation(_owner, _animationTimeLength, _iFunc)
{
    var = _var;
    endValue = _endValue;
}
/*
template<class T>
LinearAnimation<T>::LinearAnimation(AnimatedObject * _owner, T * _var, T _endValue, float32 _fixedSpeed)
	: Animation(_owner, 0.0f, Interpolation::LINEAR)
{
	var = _var;
	endValue = _endValue;
	
}
*/

template <class T>
void LinearAnimation<T>::OnStart()
{
    startValue = *var;
}

template <class T>
void LinearAnimation<T>::Update(float32 timeElapsed)
{
    Animation::Update(timeElapsed);
    *var = startValue + (endValue - startValue) * normalizedTime;
}

template <>
inline //Dizz: inline tells compiler that symbol will be present in multiple object files
void
LinearAnimation<int32>::Update(float32 timeElapsed)
{
    Animation::Update(timeElapsed);
    *var = startValue + static_cast<int32>(static_cast<float32>(endValue - startValue) * normalizedTime);
}

//template<Rect>

class RectLinearAnimation : public Animation
{
protected:
    ~RectLinearAnimation()
    {
    }

public:
    RectLinearAnimation(AnimatedObject* _owner, Rect* _var, Rect _endValue, float32 _animationTimeLength, Interpolation::FuncType _iType);
    virtual void Update(float32 timeElapsed);
    virtual void OnStart();

private:
    Rect* var;
    Rect endValue;
    Rect startValue;
};

class TwoVector2LinearAnimation : public Animation
{
protected:
    ~TwoVector2LinearAnimation()
    {
    }

public:
    TwoVector2LinearAnimation(AnimatedObject* _owner, Vector2* _var1, Vector2 _endValue1, Vector2* _var2, Vector2 _endValue2, float32 _animationTimeLength, Interpolation::FuncType _iType);
    virtual void Update(float32 timeElapsed);
    virtual void OnStart();

private:
    Vector2* var1;
    Vector2 endValue1;
    Vector2 startValue1;
    Vector2* var2;
    Vector2 endValue2;
    Vector2 startValue2;
};
};
#endif // __DAVAENGINE_LINEAR_ANIMATION_H__