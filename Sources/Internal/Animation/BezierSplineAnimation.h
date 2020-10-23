#ifndef __DAVAENGINE_BEZIER_SPLINE_ANIMATION_H__
#define __DAVAENGINE_BEZIER_SPLINE_ANIMATION_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/BaseObject.h"
#include "Logger/Logger.h"
#include "Animation/Animation.h"
#include "Animation/AnimatedObject.h"

namespace DAVA
{
class BezierSplineAnimation2 : public Animation
{
protected:
    ~BezierSplineAnimation2()
    {
    }

public:
    BezierSplineAnimation2(AnimatedObject* _owner, Vector2* _var, BezierSpline2* _spline, float32 _animationTimeLength, Interpolation::FuncType _iType);
    virtual void Update(float32 timeElapsed);
    virtual void OnStart();

private:
    Vector2* var; //i hate you boroda
    BezierSpline2* spline;
};

class BezierSplineAnimation3 : public Animation
{
protected:
    ~BezierSplineAnimation3()
    {
    }

public:
    BezierSplineAnimation3(AnimatedObject* _owner, Vector3* _var, BezierSpline3* _spline, float32 _animationTimeLength, Interpolation::FuncType _iType);
    virtual void Update(float32 timeElapsed);
    virtual void OnStart();

private:
    Vector3* var; //i hate you boroda
    BezierSpline3* spline;
};
};
#endif // __DAVAENGINE_BEZIER_SPLINE_ANIMATION_H__