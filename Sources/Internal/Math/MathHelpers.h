#pragma once

#include <cmath>
#include "Math/Math2D.h"
#include "Math/Vector.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Math/MathConstants.h"

namespace DAVA
{
/*
    Radians to degrees and back conversion functions and constants
    */

static const float32 RAD_TO_DEG = 180.0f / 3.14159265358979323846f;
static const float32 DEG_TO_RAD = 3.14159265358979323846f / 180.0f;

inline float32 RadToDeg(float32 f)
{
    return f * RAD_TO_DEG;
};
inline float32 DegToRad(float32 f)
{
    return f * DEG_TO_RAD;
};

inline void SinCosFast(float angleInRadians, float& sine, float& cosine)
{
    if (angleInRadians < 0.0f || angleInRadians >= PI_2)
    {
        angleInRadians -= std::floor(angleInRadians * (1.0f / PI_2)) * PI_2;
    }
    sine = PI - angleInRadians;
    if (Abs(sine) > PI_05)
    {
        sine = ((sine > 0.0f) ? PI : -PI) - sine;
        cosine = 1.0f;
    }
    else
    {
        cosine = -1.0f;
    }
    float a2 = sine * sine;
    sine *= ((0.00761f * a2 - 0.16605f) * a2 + 1.0f);
    cosine *= ((0.03705f * a2 - 0.49670f) * a2 + 1.0f);
}

inline float32 InvSqrtFast(float32 number) //only for IEEE 754 floating point format
{
    int32 i;
    float x2, y;
    const float32 threehalfs = 1.5f;

    x2 = number * 0.5f;
    y = number;
    i = *(reinterpret_cast<int32*>(&y));
    i = 0x5f3759df - (i >> 1);
    y = *(reinterpret_cast<float32*>(&i));
    y = y * (threehalfs - (x2 * y * y));

    return y;
}

/*
    Function to conver euler angles to normalized axial vectors
*/
void AnglesToVectors(const Vector3& _angles, Vector3& _vx, Vector3& _vy, Vector3& _vz);

template <typename T>
inline bool IsPowerOf2(T value)
{
    static_assert(std::is_integral<T>::value, "IsPowerOf2 works only with integral types");
    return value != 0 && ((value & (value - 1)) == 0);
}

inline int32 NextPowerOf2(int32 x)
{
    if (IsPowerOf2(x))
        return x;

    int32 ret = 1;

    while (ret < x)
    {
        ret *= 2;
    }

    return ret;
}

inline void EnsurePowerOf2(int32& x)
{
    x = NextPowerOf2(x);
}

template <typename T>
inline size_t HighestBitIndex(T value)
{
    static_assert(std::is_integral<T>::value, "HighestBitIndex works only with integral types");
    size_t index = 0;
    if (value != 0)
    {
        using UnsignedT = typename std::make_unsigned<T>::type;
        UnsignedT unsignedValue = static_cast<UnsignedT>(value);
        for (; unsignedValue != 0; ++index)
        {
            unsignedValue >>= 1;
        }
        index -= 1;
    }
    return index;
}

template <typename T>
inline T Sign(T val)
{
    if (val == 0)
    {
        return 0;
    }
    return T(val > 0 ? 1 : -1);
}

template <typename T>
inline T Lerp(const T& a, const T& b, float32 w)
{
    return a + w * (b - a);
}

template <typename T>
inline T Lerp(T a, T b, T w)
{
    return a + w * (b - a);
}
/*
    Function to get intersection point of 
    vector (start + dir) 
    with plane (plane normal + plane point)
    */
DAVA_DEPRECATED(inline bool GetIntersectionVectorWithPlane(const Vector3& start, const Vector3& dir, const Vector3& planeN, const Vector3& planePoint, Vector3& result));
inline bool GetIntersectionVectorWithPlane(const Vector3& start, const Vector3& dir, const Vector3& planeN, const Vector3& planePoint, Vector3& result)
{
    Vector3 intersection;
    float32 cosang, dist, lamda;

    float32 d = planeN.DotProduct(planePoint);

    cosang = dir.DotProduct(planeN);
    if (cosang > -EPSILON && cosang < EPSILON)
    {
        //this is parallels
        return false;
    }

    dist = start.DotProduct(planeN);

    lamda = (d - dist) / cosang;
    if (lamda < 0)
    {
        //this not intersect
        return false;
    }
    result = start + dir * lamda;
    return true;
}

/*
    ================
    SquareRootFloat
    ================
    */
inline float32 SquareRootFloat(float32 number)
{
    int32 i;
    float32 x, y;
    const float32 f = 1.5f;

    x = number * 0.5f;
    y = number;
    i = *(reinterpret_cast<int32*>(&y));
    i = 0x5f3759df - (i >> 1);
    y = *(reinterpret_cast<float32*>(&i));
    y = y * (f - (x * y * y));
    y = y * (f - (x * y * y));
    return number * y;
}

inline Vector3& TransformPerserveLength(Vector3& vec, const Matrix3& mat)
{
    float32 oldLength = vec.SquareLength();
    vec = vec * mat;
    float newLength = vec.SquareLength();
    if (newLength > EPSILON)
        vec *= std::sqrt(oldLength / newLength);
    return vec;
}

inline Vector3& TransformPerserveLength(Vector3& vec, const Matrix4& mat)
{
    float32 oldLength = vec.SquareLength();
    vec = vec * mat;
    float newLength = vec.SquareLength();
    if (newLength > EPSILON)
        vec *= std::sqrt(oldLength / newLength);
    return vec;
}

inline float32 Round(float32 value)
{
    return (value > 0.0f) ? std::floor(value + 0.5f) : std::ceil(value - 0.5f);
}

inline Vector3 Polar(DAVA::float32 angle, DAVA::float32 distance)
{
    return DAVA::Vector3(std::cos(angle) * distance, std::sin(angle) * distance, 0.0f);
};

inline void BuildOrthonormalBasis(const Vector3& n, Vector3& u, Vector3& v)
{
    // Building an Orthonormal Basis, Revisited
    // http://jcgt.org/published/0006/01/01/
    if (n.z < 0.0f)
    {
        float a = 1.0f / (1.0f - n.z);
        float b = n.x * n.y * a;
        u = Vector3(n.x * n.x * a - 1.0f, b, -n.x);
        v = Vector3(b, n.y * n.y * a - 1.0f, -n.y);
    }
    else
    {
        float a = 1.0f / (1.0f + n.z);
        float b = -n.x * n.y * a;
        u = Vector3(1.0f - n.x * n.x * a, b, -n.x);
        v = Vector3(b, 1.0f - n.y * n.y * a, -n.y);
    }
}

inline Matrix4 BuildOrientedMatrix(const Vector3& direction, const Vector3& translationVector)
{
    Vector3 n = -direction;
    n.Normalize();

    Vector3 u;
    Vector3 v;
    BuildOrthonormalBasis(n, u, v);

    Matrix4 t = Matrix4::IDENTITY;
    t._data[0][0] = u.x;
    t._data[0][1] = v.x;
    t._data[0][2] = -n.x;
    t._data[1][0] = u.y;
    t._data[1][1] = v.y;
    t._data[1][2] = -n.y;
    t._data[2][0] = u.z;
    t._data[2][1] = v.z;
    t._data[2][2] = -n.z;
    t.SetTranslationVector(translationVector);

    return t;
}

inline Matrix4 Convert2DTransformTo3DTransform(const Matrix3& m2d)
{
    return Matrix4(
    m2d._00, m2d._01, 0.f, 0.f,
    m2d._10, m2d._11, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    m2d._20, m2d._21, 0.f, 1.f
    );
}

/**
 * Removes scale from original matrix and calculates it inverse via transpose.
*/
inline Matrix4 GetInverseWithRemovedScale(const Matrix4& mat)
{
    float32 invScale = 1.0f / std::sqrt(mat._00 * mat._00 + mat._01 * mat._01 + mat._02 * mat._02);
    Matrix4 inverse
    (
    mat._00 * invScale, mat._10 * invScale, mat._20 * invScale, 0.0f,
    mat._01 * invScale, mat._11 * invScale, mat._21 * invScale, 0.0f,
    mat._02 * invScale, mat._12 * invScale, mat._22 * invScale, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
    );
    Vector3 t = mat.GetTranslationVector();
    Vector3 translation = -Vector3
                          (
                          t.x * inverse._00 + t.y * inverse._10 + t.z * inverse._20,
                          t.x * inverse._01 + t.y * inverse._11 + t.z * inverse._21,
                          t.x * inverse._02 + t.y * inverse._12 + t.z * inverse._22
                          );
    inverse.SetTranslationVector(translation);
    return inverse;
}

inline float32 Step(float32 edge, float32 value)
{
    return (value >= edge) ? 1.0f : 0.0f;
}

inline Vector4 Step(const Vector4& edge, const Vector4& value)
{
    return Vector4
    (
    Step(edge.x, value.x),
    Step(edge.y, value.y),
    Step(edge.z, value.z),
    Step(edge.w, value.w)
    );
}
} // end of namespace DAVA
