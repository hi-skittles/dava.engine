#include "Base/BaseMath.h"
#include "Animation/LinearAnimation.h"

namespace DAVA
{
RectLinearAnimation::RectLinearAnimation(AnimatedObject* _owner, Rect* _var, Rect _endValue, float32 _animationTimeLength, Interpolation::FuncType _iType)
    : Animation(_owner, _animationTimeLength, _iType)
{
    var = _var;
    endValue = _endValue;
}

void RectLinearAnimation::OnStart()
{
    startValue = *var;
}

void RectLinearAnimation::Update(float32 timeElapsed)
{
    Animation::Update(timeElapsed);
    var->x = startValue.x + (endValue.x - startValue.x) * normalizedTime;
    var->y = startValue.y + (endValue.y - startValue.y) * normalizedTime;
    var->dx = startValue.dx + (endValue.dx - startValue.dx) * normalizedTime;
    var->dy = startValue.dy + (endValue.dy - startValue.dy) * normalizedTime;
}

TwoVector2LinearAnimation::TwoVector2LinearAnimation(AnimatedObject* _owner, Vector2* _var1, Vector2 _endValue1, Vector2* _var2, Vector2 _endValue2, float32 _animationTimeLength, Interpolation::FuncType _iType)
    : Animation(_owner, _animationTimeLength, _iType)
{
    var1 = _var1;
    var2 = _var2;
    endValue1 = _endValue1;
    endValue2 = _endValue2;
}
void TwoVector2LinearAnimation::Update(float32 timeElapsed)
{
    Animation::Update(timeElapsed);
    *var1 = startValue1 + (endValue1 - startValue1) * normalizedTime;
    *var2 = startValue2 + (endValue2 - startValue2) * normalizedTime;
}
void TwoVector2LinearAnimation::OnStart()
{
    startValue1 = *var1;
    startValue2 = *var2;
}
};