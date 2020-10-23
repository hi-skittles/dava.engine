#include "Animation/LinearPropertyAnimation.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
ReflectionAnyPropertyAnimation::ReflectionAnyPropertyAnimation(AnimatedObject* _owner, const Reflection& _ref, float32 _animationTimeLength, Interpolation::FuncType _iFuncType)
    : Animation(_owner, _animationTimeLength, _iFuncType)
    , ref(std::make_unique<Reflection>(_ref))
{
}

ReflectionAnyPropertyAnimation::~ReflectionAnyPropertyAnimation() = default;

void ReflectionAnyPropertyAnimation::SetPropertyValue(const Any& value)
{
    ref->SetValueWithCast(value);
}
}
