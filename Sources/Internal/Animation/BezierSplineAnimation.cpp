#include "Animation/BezierSplineAnimation.h"

namespace DAVA
{
BezierSplineAnimation2::BezierSplineAnimation2(AnimatedObject* _owner, Vector2* _var, BezierSpline2* _spline, float32 _animationTimeLength, Interpolation::FuncType _iType)
    : Animation(_owner, _animationTimeLength, _iType)
{
    var = _var;
    spline = _spline;
}

void BezierSplineAnimation2::OnStart()
{
    //startValue = *var;
}

void BezierSplineAnimation2::Update(float32 timeElapsed)
{
    Animation::Update(timeElapsed);
    *var = spline->Evaluate(0, normalizedTime);
}

BezierSplineAnimation3::BezierSplineAnimation3(AnimatedObject* _owner, Vector3* _var, BezierSpline3* _spline, float32 _animationTimeLength, Interpolation::FuncType _iType)
    : Animation(_owner, _animationTimeLength, _iType)
{
    var = _var;
    spline = _spline;
}

void BezierSplineAnimation3::OnStart()
{
    //startValue = *var;
}

void BezierSplineAnimation3::Update(float32 timeElapsed)
{
    Animation::Update(timeElapsed);
    *var = spline->Evaluate(0, normalizedTime);
}
};