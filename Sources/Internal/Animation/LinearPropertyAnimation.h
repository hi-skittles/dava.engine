#ifndef __DAVAENGINE_LINEAR_PROPERTY_ANIMATION_H__
#define __DAVAENGINE_LINEAR_PROPERTY_ANIMATION_H__

#include "Animation/Animation.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
class Reflection;
class ReflectionAnyPropertyAnimation : public Animation
{
public:
    ReflectionAnyPropertyAnimation(AnimatedObject* _owner, const Reflection& _ref, float32 _animationTimeLength, Interpolation::FuncType _iFuncType);

protected:
    ~ReflectionAnyPropertyAnimation();
    void SetPropertyValue(const Any& value);

private:
    std::unique_ptr<Reflection> ref;
};

template <class T>
class LinearPropertyAnimation : public ReflectionAnyPropertyAnimation
{
protected:
    ~LinearPropertyAnimation()
    {
    }

public:
    LinearPropertyAnimation(AnimatedObject* _owner, const Reflection& _ref, const T& _startValue, const T& _endValue, float32 _animationTimeLength, Interpolation::FuncType _iFuncType);

    virtual void Update(float32 timeElapsed);

    const T& GetStartValue() const;
    const T& GetEndValue() const;

protected:
    T startValue;
    T endValue;
};

template <class T>
LinearPropertyAnimation<T>::LinearPropertyAnimation(AnimatedObject* _owner, const Reflection& _ref, const T& _startValue, const T& _endValue, float32 _animationTimeLength, Interpolation::FuncType _iFuncType)
    : ReflectionAnyPropertyAnimation(_owner, _ref, _animationTimeLength, _iFuncType)
    , startValue(_startValue)
    , endValue(_endValue)
{
}

template <class T>
void LinearPropertyAnimation<T>::Update(float32 timeElapsed)
{
    ReflectionAnyPropertyAnimation::Update(timeElapsed);
    T val = startValue + (endValue - startValue) * normalizedTime;
    SetPropertyValue(val);
}

template <class T>
const T& LinearPropertyAnimation<T>::GetStartValue() const
{
    return startValue;
}

template <class T>
const T& LinearPropertyAnimation<T>::GetEndValue() const
{
    return endValue;
}
};

#endif
