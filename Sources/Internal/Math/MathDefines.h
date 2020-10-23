#pragma once

#include <cmath>
#include "Base/BaseTypes.h"
#include "Math/MathConstants.h"

/*
 It contein's defines from Math2D.h to resolve circular reference
 between Math2D.h and Math2DTemplateClasses.h
 */

namespace DAVA
{
    
#define FLOAT_EQUAL(f1, f2) (std::abs(f1 - f2) < DAVA::EPSILON)
#define FLOAT_EQUAL_EPS(f1, f2, EPS) (std::abs(f1 - f2) < EPS)
    
#define VECTOR_EQUAL(v1, v2) (FLOAT_EQUAL(v1.x, v2.x) && FLOAT_EQUAL(v1.y, v2.y) && FLOAT_EQUAL(v1.z, v2.z))
#define VECTOR_EQUAL_EPS(v1, v2, EPS) (FLOAT_EQUAL_EPS(v1.x, v2.x, EPS) && FLOAT_EQUAL_EPS(v1.y, v2.y, EPS) && FLOAT_EQUAL_EPS(v1.z, v2.z, EPS))

#define QUATERNION_EQUAL(q1, q2) (FLOAT_EQUAL(q1.x, q2.x) && FLOAT_EQUAL(q1.y, q2.y) && FLOAT_EQUAL(q1.z, q2.z) && FLOAT_EQUAL(q1.w, q2.w))
#define QUATERNION_EQUAL_EPS(q1, q2, EPS) (FLOAT_EQUAL_EPS(q1.x, q2.x, EPS) && FLOAT_EQUAL_EPS(q1.y, q2.y, EPS) \
                                           && FLOAT_EQUAL_EPS(q1.z, q2.z, EPS) && FLOAT_EQUAL_EPS(q1.w, q2.w, EPS))
    
    
#define TRANSFORM_EQUAL_EPS(t1, t2, EPS) (VECTOR_EQUAL_EPS(t1.GetTranslation(), t2.GetTranslation(), EPS) \
                                          && VECTOR_EQUAL_EPS(t1.GetScale(), t2.GetScale(), EPS) \
                                          && QUATERNION_EQUAL_EPS(t1.GetRotation(), t2.GetRotation(), EPS))
}
