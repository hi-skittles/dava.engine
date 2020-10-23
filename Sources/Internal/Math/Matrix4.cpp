#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Math/Matrix4.h"
#include "Math/Quaternion.h"
#include "Logger/Logger.h"

namespace DAVA
{
Matrix4 Matrix4::IDENTITY(1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, 1.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1.0f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f);

#if 0

void Matrix4::glRotate(float32 angle, float32 x, float32 y, float32 z)
{
    float32 xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c, s, c;
    bool optimized = false;

    s = std::sin(DegToRad(angle));
    c = std::cos(DegToRad(angle));

    memcpy(this->data, IDENTITY.data, sizeof(float32) * 16);
    
#define M(row, col) this->data[col * 4 + row]

    if (x == 0.0F)
    {
        if (y == 0.0F)
        {
            if (z != 0.0F)
            {
                optimized = true;
                /* rotate only around z-axis */
                M(0, 0) = c;
                M(1, 1) = c;
                if (z < 0.0F)
                {
                    M(0, 1) = s;
                    M(1, 0) = -s;
                }
                else
                {
                    M(0, 1) = -s;
                    M(1, 0) = s;
                }
            }
        }
        else if (z == 0.0F)
        {
            optimized = true;
            /* rotate only around y-axis */
            M(0, 0) = c;
            M(2, 2) = c;
            if (y < 0.0F)
            {
                M(0, 2) = -s;
                M(2, 0) = s;
            }
            else
            {
                M(0, 2) = s;
                M(2, 0) = -s;
            }
        }
    }
    else if (y == 0.0F)
    {
        if (z == 0.0F)
        {
            optimized = true;
            /* rotate only around x-axis */
            M(1, 1) = c;
            M(2, 2) = c;
            if (x < 0.0F)
            {
                M(1, 2) = s;
                M(2, 1) = -s;
            }
            else
            {
                M(1, 2) = -s;
                M(2, 1) = s;
            }
        }
    }

    if (!optimized)
    {
        const float32 mag = std::sqrt(x * x + y * y + z * z);

        if (mag <= 1.0e-4)
        {
            /* no rotation, leave mat as-is */
            return;
        }

        x /= mag;
        y /= mag;
        z /= mag;

        /*
         *     Arbitrary axis rotation matrix.
         *
         *  This is composed of 5 matrices, Rz, Ry, T, Ry', Rz', multiplied
         *  like so:  Rz * Ry * T * Ry' * Rz'.  T is the final rotation
         *  (which is about the X-axis), and the two composite transforms
         *  Ry' * Rz' and Rz * Ry are (respectively) the rotations necessary
         *  from the arbitrary axis to the X-axis then back.  They are
         *  all elementary rotations.
         *
         *  Rz' is a rotation about the Z-axis, to bring the axis vector
         *  into the x-z plane.  Then Ry' is applied, rotating about the
         *  Y-axis to bring the axis vector parallel with the X-axis.  The
         *  rotation about the X-axis is then performed.  Ry and Rz are
         *  simply the respective inverse transforms to bring the arbitrary
         *  axis back to its original orientation.  The first transforms
         *  Rz' and Ry' are considered inverses, since the data from the
         *  arbitrary axis gives you info on how to get to it, not how
         *  to get away from it, and an inverse must be applied.
         *
         *  The basic calculation used is to recognize that the arbitrary
         *  axis vector (x, y, z), since it is of unit length, actually
         *  represents the sines and cosines of the angles to rotate the
         *  X-axis to the same orientation, with theta being the angle about
         *  Z and phi the angle about Y (in the order described above)
         *  as follows:
         *
         *  cos ( theta ) = x / sqrt ( 1 - z^2 )
         *  sin ( theta ) = y / sqrt ( 1 - z^2 )
         *
         *  cos ( phi ) = sqrt ( 1 - z^2 )
         *  sin ( phi ) = z
         *
         *  Note that cos ( phi ) can further be inserted to the above
         *  formulas:
         *
         *  cos ( theta ) = x / cos ( phi )
         *  sin ( theta ) = y / sin ( phi )
         *
         *  ...etc.  Because of those relations and the standard trigonometric
         *  relations, it is pssible to reduce the transforms down to what
         *  is used below.  It may be that any primary axis chosen will give the
         *  same results (modulo a sign convention) using thie method.
         *
         *  Particularly nice is to notice that all divisions that might
         *  have caused trouble when parallel to certain planes or
         *  axis go away with care paid to reducing the expressions.
         *  After checking, it does perform correctly under all cases, since
         *  in all the cases of division where the denominator would have
         *  been zero, the numerator would have been zero as well, giving
         *  the expected result.
         */

        xx = x * x;
        yy = y * y;
        zz = z * z;
        xy = x * y;
        yz = y * z;
        zx = z * x;
        xs = x * s;
        ys = y * s;
        zs = z * s;
        one_c = 1.0F - c;

        /* We already hold the identity-matrix so we can skip some statements */
        M(0, 0) = (one_c * xx) + c;
        M(0, 1) = (one_c * xy) - zs;
        M(0, 2) = (one_c * zx) + ys;
        /*    M(0,3) = 0.0F; */

        M(1, 0) = (one_c * xy) + zs;
        M(1, 1) = (one_c * yy) + c;
        M(1, 2) = (one_c * yz) - xs;
        /*    M(1,3) = 0.0F; */

        M(2, 0) = (one_c * zx) - ys;
        M(2, 1) = (one_c * yz) + xs;
        M(2, 2) = (one_c * zz) + c;
        /*    M(2,3) = 0.0F; */

        /*
         M(3,0) = 0.0F;
         M(3,1) = 0.0F;
         M(3,2) = 0.0F;
         M(3,3) = 1.0F;
         */
    }
#undef M
}

#endif

void Matrix4::Dump()
{
    Logger::FrameworkDebug("%5.5f %5.5f %5.5f %5.5f ", _00, _01, _02, _03);
    Logger::FrameworkDebug("%5.5f %5.5f %5.5f %5.5f ", _10, _11, _12, _13);
    Logger::FrameworkDebug("%5.5f %5.5f %5.5f %5.5f ", _20, _21, _22, _23);
    Logger::FrameworkDebug("%5.5f %5.5f %5.5f %5.5f ", _30, _31, _32, _33);
}

void Matrix4::Decomposition(Vector3& position, Vector3& scale, Quaternion& rot) const
{
    scale = GetScaleVector();
    position = GetTranslationVector();

    Matrix4 unscaled(*this);
    for (int32 i = 0; i < 3; ++i)
    {
        unscaled._data[0][i] /= scale.x;
        unscaled._data[1][i] /= scale.y;
        unscaled._data[2][i] /= scale.z;
    }
    rot.Construct(unscaled);
}

Quaternion Matrix4::GetRotation() const
{
    Vector3 scale = GetScaleVector();
    Quaternion rot;

    Matrix4 unscaled(*this);
    for (int32 i = 0; i < 3; ++i)
    {
        unscaled._data[0][i] /= scale.x;
        unscaled._data[1][i] /= scale.y;
        unscaled._data[2][i] /= scale.z;
    }
    rot.Construct(unscaled);

    return rot;
}

template <>
bool AnyCompare<Matrix4>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2)
{
    return v1.Get<Matrix4>() == v2.Get<Matrix4>();
}
};
