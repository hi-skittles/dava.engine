#pragma once

#include <cmath>
#include "Base/BaseTypes.h"
#include "Base/Any.h"
#include "Math/Vector.h"
#include "Math/MathDefines.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Class to work with 3 x 3 matrices.
 */
struct Matrix3
{
    union
    {
        float32 data[9];
        float32 _data[3][3];
        struct
        {
            float32 _00, _01, _02;
            float32 _10, _11, _12;
            float32 _20, _21, _22;
        };
    };

    inline Matrix3();

    inline Matrix3(float32 __00, float32 __01, float32 __02,
                   float32 __10, float32 __11, float32 __12,
                   float32 __20, float32 __21, float32 __22);

    inline Matrix3(const Matrix3& m);

    inline float32 Det() const;

    // Helpers
    inline void Identity();
    inline void CreateRotation(const Vector3& r, float32 angleInRadians);
    inline void BuildRotation(float32 cosA, float32 sinA);
    inline void BuildRotation(float32 angle);
    inline void BuildTranslation(float32 x, float32 y);
    inline void BuildTranslation(const Vector2& vec);
    inline void BuildScale(const Vector2& vec);
    inline bool GetInverse(Matrix3& out, float32 fTolerance = 1e-06) const;
    inline void Transpose();
    inline void Decomposition(Matrix3& kQ, Vector3& kD, Vector3& kU) const;

    inline Matrix3& operator=(const Matrix3& arg);

    inline Matrix3& operator*=(const Matrix3& arg);
    inline Matrix3 operator*(const Matrix3& arg) const;

    inline Matrix3& operator-=(const Matrix3& arg);
    inline Matrix3 operator-(const Matrix3& arg) const;

    inline Matrix3& operator+=(const Matrix3& arg);
    inline Matrix3 operator+(const Matrix3& arg) const;

    inline bool operator==(const Matrix3& _m) const;
    inline bool operator!=(const Matrix3& _m) const;
};

inline Vector2 operator*(const Vector2& _v, const Matrix3& _m);

// Implementation

inline Matrix3::Matrix3(float32 __00, float32 __01, float32 __02,
                        float32 __10, float32 __11, float32 __12,
                        float32 __20, float32 __21, float32 __22)
{
    _00 = __00;
    _01 = __01;
    _02 = __02;
    _10 = __10;
    _11 = __11;
    _12 = __12;
    _20 = __20;
    _21 = __21;
    _22 = __22;
};

inline Matrix3::Matrix3(const Matrix3& m)
{
    *this = m;
}

inline float32 Matrix3::Det() const
{
    return _00 * _11 * _22 + _01 * _12 * _20 + _02 * _10 * _21
    - _02 * _11 * _20 - _01 * _10 * _22 - _00 * _12 * _21;
}

inline Matrix3::Matrix3()
{
    _00 = 1.0f;
    _01 = 0.0f;
    _02 = 0.0f;
    _10 = 0.0f;
    _11 = 1.0f;
    _12 = 0.0f;
    _20 = 0.0f;
    _21 = 0.0f;
    _22 = 1.0f;
}

inline void Matrix3::Identity()
{
    _00 = 1.0f;
    _01 = 0.0f;
    _02 = 0.0f;
    _10 = 0.0f;
    _11 = 1.0f;
    _12 = 0.0f;
    _20 = 0.0f;
    _21 = 0.0f;
    _22 = 1.0f;
}

inline void Matrix3::CreateRotation(const Vector3& r, float32 angleInRadians)
{
    float32 cosA = std::cos(angleInRadians);
    float32 sinA = std::sin(angleInRadians);
    Identity();
    _data[0][0] = cosA + (1 - cosA) * r.x * r.x;
    _data[0][1] = (1 - cosA) * r.x * r.y - r.z * sinA;
    _data[0][2] = (1 - cosA) * r.x * r.z + r.y * sinA;

    _data[1][0] = (1 - cosA) * r.x * r.y + r.z * sinA;
    _data[1][1] = cosA + (1 - cosA) * r.y * r.y;
    _data[1][2] = (1 - cosA) * r.y * r.z - r.x * sinA;

    _data[2][0] = (1 - cosA) * r.x * r.z - r.y * sinA;
    _data[2][1] = (1 - cosA) * r.y * r.z + r.x * sinA;
    _data[2][2] = cosA + (1 - cosA) * r.z * r.z;
}

inline void Matrix3::BuildRotation(float32 cosA, float32 sinA)
{
    _00 = cosA;
    _01 = sinA;
    _02 = 0;
    _10 = -sinA;
    _11 = cosA;
    _12 = 0;
    _20 = 0;
    _21 = 0;
    _22 = 1;
}

inline void Matrix3::BuildRotation(float32 angle)
{
    float32 cosA = std::cos(angle);
    float32 sinA = std::sin(angle);

    BuildRotation(cosA, sinA);
}

inline void Matrix3::BuildTranslation(float32 _xT, float32 _yT)
{
    Identity();
    _20 = _xT;
    _21 = _yT;
}

inline void Matrix3::BuildTranslation(const Vector2& vec)
{
    Identity();
    _20 = vec.x;
    _21 = vec.y;
}

inline void Matrix3::BuildScale(const Vector2& vec)
{
    Identity();
    _00 = vec.x;
    _11 = vec.y;
}

inline Matrix3& Matrix3::operator=(const Matrix3& arg)
{
    _00 = arg._00;
    _01 = arg._01;
    _02 = arg._02;
    _10 = arg._10;
    _11 = arg._11;
    _12 = arg._12;
    _20 = arg._20;
    _21 = arg._21;
    _22 = arg._22;
    return *this;
}

inline Matrix3 Matrix3::operator*(const Matrix3& m) const
{
    return Matrix3(_00 * m._00 + _01 * m._10 + _02 * m._20,
                   _00 * m._01 + _01 * m._11 + _02 * m._21,
                   _00 * m._02 + _01 * m._12 + _02 * m._22,

                   _10 * m._00 + _11 * m._10 + _12 * m._20,
                   _10 * m._01 + _11 * m._11 + _12 * m._21,
                   _10 * m._02 + _11 * m._12 + _12 * m._22,

                   _20 * m._00 + _21 * m._10 + _22 * m._20,
                   _20 * m._01 + _21 * m._11 + _22 * m._21,
                   _20 * m._02 + _21 * m._12 + _22 * m._22);
}

inline Matrix3& Matrix3::operator*=(const Matrix3& m)
{
    return (*this = *this * m);
}

inline Matrix3 Matrix3::operator+(const Matrix3& m) const
{
    return Matrix3(_00 + m._00, _01 + m._01, _02 + m._02,
                   _10 + m._10, _11 + m._11, _12 + m._12,
                   _20 + m._20, _21 + m._21, _22 + m._22);
}

inline Matrix3& Matrix3::operator+=(const Matrix3& m)
{
    return (*this = *this + m);
}

inline Matrix3 Matrix3::operator-(const Matrix3& m) const
{
    return Matrix3(_00 - m._00, _01 - m._01, _02 - m._02,
                   _10 - m._10, _11 - m._11, _12 - m._12,
                   _20 - m._20, _21 - m._21, _22 - m._22);
}

inline Matrix3& Matrix3::operator-=(const Matrix3& m)
{
    return (*this = *this - m);
}

inline Vector2 operator*(const Vector2& _v, const Matrix3& _m)
{
    Vector2 res;

    res.x = _v.x * _m._00 + _v.y * _m._10 + _m._20;
    res.y = _v.x * _m._01 + _v.y * _m._11 + _m._21;

    return res;
}

inline Vector3 operator*(const Vector3& _v, const Matrix3& _m)
{
    Vector3 res;

    res.x = _v.x * _m._00 + _v.y * _m._10 + _v.z * _m._20;
    res.y = _v.x * _m._01 + _v.y * _m._11 + _v.z * _m._21;
    res.z = _v.x * _m._02 + _v.y * _m._12 + _v.z * _m._22;

    return res;
}

/*
	 rkInverse[0][0] = m[1][1]*m[2][2] -
	 m[1][2]*m[2][1];
	 rkInverse[0][1] = m[0][2]*m[2][1] -
	 m[0][1]*m[2][2];
	 rkInverse[0][2] = m[0][1]*m[1][2] -
	 m[0][2]*m[1][1];
	 rkInverse[1][0] = m[1][2]*m[2][0] -
	 m[1][0]*m[2][2];
	 rkInverse[1][1] = m[0][0]*m[2][2] -
	 m[0][2]*m[2][0];
	 rkInverse[1][2] = m[0][2]*m[1][0] -
	 m[0][0]*m[1][2];
	 rkInverse[2][0] = m[1][0]*m[2][1] -
	 m[1][1]*m[2][0];
	 rkInverse[2][1] = m[0][1]*m[2][0] -
	 m[0][0]*m[2][1];
	 rkInverse[2][2] = m[0][0]*m[1][1] -
	 m[0][1]*m[1][0];
	 
	 Real fDet =
	 m[0][0]*rkInverse[0][0] +
	 m[0][1]*rkInverse[1][0]+
	 m[0][2]*rkInverse[2][0];
	 
	 if ( Math::Abs(fDet) <= fTolerance )
	 return false;
	 
	 Real fInvDet = 1.0f/fDet;
	 for (size_t iRow = 0; iRow < 3; iRow++)
	 {
	 for (size_t iCol = 0; iCol < 3; iCol++)
	 rkInverse[iRow][iCol] *= fInvDet;
	 }

        return true;

	 */

inline bool Matrix3::GetInverse(Matrix3& out, float32 fTolerance) const
{
    const Matrix3& m = *this;
    out._00 = m._data[1][1] * m._data[2][2] - m._data[1][2] * m._data[2][1];
    out._01 = m._data[0][2] * m._data[2][1] - m._data[0][1] * m._data[2][2];
    out._02 = m._data[0][1] * m._data[1][2] - m._data[0][2] * m._data[1][1];
    out._10 = m._data[1][2] * m._data[2][0] - m._data[1][0] * m._data[2][2];
    out._11 = m._data[0][0] * m._data[2][2] - m._data[0][2] * m._data[2][0];
    out._12 = m._data[0][2] * m._data[1][0] - m._data[0][0] * m._data[1][2];
    out._20 = m._data[1][0] * m._data[2][1] - m._data[1][1] * m._data[2][0];
    out._21 = m._data[0][1] * m._data[2][0] - m._data[0][0] * m._data[2][1];
    out._22 = m._data[0][0] * m._data[1][1] - m._data[0][1] * m._data[1][0];

    float32 fDet = m._data[0][0] * out._data[0][0] + m._data[0][1] * out._data[1][0] + m._data[0][2] * out._data[2][0];

    if (Abs(fDet) <= fTolerance)
        return false;

    float32 fInvDet = 1.0f / fDet;
    for (int32 iRow = 0; iRow < 9; iRow++)
    {
        out.data[iRow] *= fInvDet;
    }

    return true;
}

inline void Matrix3::Transpose()
{
    Matrix3 t;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            t._data[i][j] = _data[j][i];
    *this = t;
}

inline void Matrix3::Decomposition(Matrix3& kQ, Vector3& kD, Vector3& kU) const
{
    // Factor M = QR = QDU where Q is orthogonal, D is diagonal,
    // and U is upper triangular with ones on its diagonal.  Algorithm uses
    // Gram-Schmidt orthogonalization (the QR algorithm).
    //
    // If M = [ m0 | m1 | m2 ] and Q = [ q0 | q1 | q2 ], then
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.  The matrix R has entries
    //
    //   r00 = q0*m0  r01 = q0*m1  r02 = q0*m2
    //   r10 = 0      r11 = q1*m1  r12 = q1*m2
    //   r20 = 0      r21 = 0      r22 = q2*m2
    //
    // so D = diag(r00,r11,r22) and U has entries u01 = r01/r00,
    // u02 = r02/r00, and u12 = r12/r11.

    // Q = rotation
    // D = scaling
    // U = shear

    // D stores the three diagonal entries r00, r11, r22
    // U stores the entries U[0] = u01, U[1] = u02, U[2] = u12

    // build orthogonal matrix Q
    float32 fInvLength = std::sqrt(_data[0][0] * _data[0][0] + _data[1][0] * _data[1][0] + _data[2][0] * _data[2][0]);

    kQ._data[0][0] = _data[0][0] * fInvLength;
    kQ._data[1][0] = _data[1][0] * fInvLength;
    kQ._data[2][0] = _data[2][0] * fInvLength;

    float32 fDot = kQ._data[0][0] * _data[0][1] + kQ._data[1][0] * _data[1][1] +
    kQ._data[2][0] * _data[2][1];
    kQ._data[0][1] = _data[0][1] - fDot * kQ._data[0][0];
    kQ._data[1][1] = _data[1][1] - fDot * kQ._data[1][0];
    kQ._data[2][1] = _data[2][1] - fDot * kQ._data[2][0];
    fInvLength = std::sqrt(kQ._data[0][1] * kQ._data[0][1] + kQ._data[1][1] * kQ._data[1][1] + kQ._data[2][1] * kQ._data[2][1]);

    kQ._data[0][1] *= fInvLength;
    kQ._data[1][1] *= fInvLength;
    kQ._data[2][1] *= fInvLength;

    fDot = kQ._data[0][0] * _data[0][2] + kQ._data[1][0] * _data[1][2] +
    kQ._data[2][0] * _data[2][2];
    kQ._data[0][2] = _data[0][2] - fDot * kQ._data[0][0];
    kQ._data[1][2] = _data[1][2] - fDot * kQ._data[1][0];
    kQ._data[2][2] = _data[2][2] - fDot * kQ._data[2][0];
    fDot = kQ._data[0][1] * _data[0][2] + kQ._data[1][1] * _data[1][2] +
    kQ._data[2][1] * _data[2][2];
    kQ._data[0][2] -= fDot * kQ._data[0][1];
    kQ._data[1][2] -= fDot * kQ._data[1][1];
    kQ._data[2][2] -= fDot * kQ._data[2][1];
    fInvLength = std::sqrt(kQ._data[0][2] * kQ._data[0][2] + kQ._data[1][2] * kQ._data[1][2] + kQ._data[2][2] * kQ._data[2][2]);

    kQ._data[0][2] *= fInvLength;
    kQ._data[1][2] *= fInvLength;
    kQ._data[2][2] *= fInvLength;

    // guarantee that orthogonal matrix has determinant 1 (no reflections)
    float32 fDet = kQ._data[0][0] * kQ._data[1][1] * kQ._data[2][2] + kQ._data[0][1] * kQ._data[1][2] * kQ._data[2][0] +
    kQ._data[0][2] * kQ._data[1][0] * kQ._data[2][1] - kQ._data[0][2] * kQ._data[1][1] * kQ._data[2][0] -
    kQ._data[0][1] * kQ._data[1][0] * kQ._data[2][2] - kQ._data[0][0] * kQ._data[1][2] * kQ._data[2][1];

    if (fDet < 0.0f)
    {
        for (size_t iRow = 0; iRow < 3; iRow++)
            for (size_t iCol = 0; iCol < 3; iCol++)
                kQ._data[iRow][iCol] = -kQ._data[iRow][iCol];
    }

    // build "right" matrix R
    Matrix3 kR;
    kR._data[0][0] = kQ._data[0][0] * _data[0][0] + kQ._data[1][0] * _data[1][0] +
    kQ._data[2][0] * _data[2][0];
    kR._data[0][1] = kQ._data[0][0] * _data[0][1] + kQ._data[1][0] * _data[1][1] +
    kQ._data[2][0] * _data[2][1];
    kR._data[1][1] = kQ._data[0][1] * _data[0][1] + kQ._data[1][1] * _data[1][1] +
    kQ._data[2][1] * _data[2][1];
    kR._data[0][2] = kQ._data[0][0] * _data[0][2] + kQ._data[1][0] * _data[1][2] +
    kQ._data[2][0] * _data[2][2];
    kR._data[1][2] = kQ._data[0][1] * _data[0][2] + kQ._data[1][1] * _data[1][2] +
    kQ._data[2][1] * _data[2][2];
    kR._data[2][2] = kQ._data[0][2] * _data[0][2] + kQ._data[1][2] * _data[1][2] +
    kQ._data[2][2] * _data[2][2];

    // the scaling component
    kD.x = kR._data[0][0];
    kD.y = kR._data[1][1];
    kD.z = kR._data[2][2];

    // the shear component
    float32 fInvD0 = 1.0f / kD.x;
    kU.x = kR._data[0][1] * fInvD0;
    kU.y = kR._data[0][2] * fInvD0;
    kU.z = kR._data[1][2] / kD.y;
}

inline bool Matrix3::operator==(const Matrix3& _m) const
{
    for (uint8 k = 0; k < 9; ++k)
    {
        if (!FLOAT_EQUAL(data[k], _m.data[k]))
        {
            return false;
        }
    }
    return true;
}

inline bool Matrix3::operator!=(const Matrix3& _m) const
{
    return !Matrix3::operator==(_m);
}

template <>
bool AnyCompare<Matrix3>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<Matrix3>;

}; // end of namespace DAVA
