#pragma once

#include <cmath>
#include "Base/BaseTypes.h"
#include "Base/Any.h"
#include "Math/MathConstants.h"

namespace DAVA
{
/**	
    \ingroup math
    \brief Vector with 2 coordinates.
 */
class Vector2
{
public:
    enum eAxis
    {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_COUNT = 2
    };
    static const Vector2 Zero;
    static const Vector2 UnitX;
    static const Vector2 UnitY;

    union {
        struct
        {
            float32 x, y;
        };
        struct
        {
            float32 dx, dy;
        };
        float32 data[AXIS_COUNT];
    };

    //! Basic
    inline Vector2();
    inline Vector2(float32 _x, float32 _y);
    inline Vector2(const float32* _data);
    inline Vector2(const Vector2& _v);

    inline Vector2& operator=(const Vector2& _v);

    //! Set functions
    inline void Set(float32 _x, float32 _y);

    //! Additional functions
    inline void Lerp(const Vector2& _v1, const Vector2& _v2, float32 t);

    //! On functions
    inline float32 SquareLength() const;
    inline float32 Length() const;
    inline void Normalize();

    //! Additional functions
    inline float32 DotProduct(const Vector2& _v2) const;
    inline float32 CrossProduct(const Vector2& b) const;
    inline float32 Angle() const; // Need to normalize vector before use function

    inline bool IsZero() const
    {
        return x == 0.f && y == 0.f;
    }
    inline void SetZero()
    {
        x = y = 0.f;
    } // = 0

    //! Get operators
    float32& operator[](eAxis axis);
    const float32 operator[](eAxis axis) const;
    //! On operations
    inline const Vector2& operator+=(const Vector2& _v);
    inline const Vector2& operator-=(const Vector2& _v);
    inline const Vector2& operator*=(const Vector2& _v);
    inline const Vector2& operator/=(const Vector2& _v);
    inline const Vector2& operator*=(float32 f);
    inline const Vector2& operator/=(float32 f);
    inline Vector2 operator-() const;

    //! Comparison operators
    inline bool operator==(const Vector2& _v) const;
    inline bool operator!=(const Vector2& _v) const;
};
//! operators
inline Vector2 operator-(const Vector2& _v1, const Vector2& _v2);
inline Vector2 operator+(const Vector2& _v1, const Vector2& _v2);
inline Vector2 operator*(const Vector2& _v1, const Vector2& _v2);
inline Vector2 operator/(const Vector2& _v1, const Vector2& _v2);

//! with scalar
inline Vector2 operator+(const Vector2& _v, float32 _f);
inline Vector2 operator+(float32 _f, const Vector2& v);

inline Vector2 operator-(const Vector2& _v, float32 _f);
inline Vector2 operator-(float32 _f, const Vector2& v);

inline Vector2 operator*(const Vector2& _v, float32 _f);
inline Vector2 operator*(float32 _f, const Vector2& v);

inline Vector2 operator/(const Vector2& _v, float32 _f);
inline Vector2 operator/(float32 _f, const Vector2& v);

//! functions
inline float32 DotProduct(const Vector2& _v1, const Vector2& _v2);
inline Vector2 Normalize(const Vector2& _v);
inline float32 CrossProduct(const Vector2& a, const Vector2& b);
inline Vector2 Reflect(const Vector2& v, const Vector2& n);
Vector2 Rotate(const Vector2& in, float32 angleRad);
/**	
    \ingroup math
    \brief Vector with 3 coordinates
 */
class Vector4;

class Vector3
{
public:
    enum eAxis : uint8
    {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2,
        AXIS_COUNT = 3
    };

public:
    static const Vector3 Zero;
    static const Vector3 UnitX;
    static const Vector3 UnitY;
    static const Vector3 UnitZ;

    union {
        struct
        {
            float32 x, y, z;
        };
        float32 data[AXIS_COUNT];
    };

    inline Vector3();
    inline Vector3(float32 _x, float32 _y, float32 _z);
    inline Vector3(const float32* _data);
    inline Vector3(const Vector2& v, float _z);
    explicit inline Vector3(const Vector2& v);
    explicit inline Vector3(const Vector4& v);
    inline Vector3(const Vector3& v);
    inline Vector3& operator=(const Vector3& _v);
    inline Vector3& operator=(const Vector2& _v);

    //! Get operators
    float32& operator[](eAxis axis);
    float32 operator[](eAxis axis) const;

    //! Set functions
    inline void Set(float32 _x, float32 _y, float32 _z);

    //! Additional functions
    inline Vector3 CrossProduct(const Vector3& _v) const;
    inline void CrossProduct(const Vector3& v1, const Vector3& v2);
    inline float32 DotProduct(const Vector3& _v) const;
    inline void Lerp(const Vector3& _v1, const Vector3& _v2, float32 t);

    inline float32 Yaw() const
    {
        return std::atan2(x, y);
    }
    inline float32 Pitch() const
    {
        return -std::atan2(z, std::sqrt(x * x + y * y));
    }

    inline bool IsZero() const
    {
        return x == 0.f && y == 0.f && z == 0.f;
    }
    inline void Zerofy()
    {
        x = y = z = 0.f;
    } // = 0

    Vector2& xy()
    {
        return *(reinterpret_cast<Vector2*>(data));
    }

    const Vector2& xy() const
    {
        return *(reinterpret_cast<const Vector2*>(data));
    }

    Vector3 xzy() const
    {
        return Vector3(x, z, y);
    }
    Vector3 zxy() const
    {
        return Vector3(z, x, y);
    }

    //! On functions
    inline float32 SquareLength() const;
    inline float32 Length() const;
    inline float32 Normalize();
    inline void Clamp(float32 min, float32 max);

    //! On operations
    inline const Vector3& operator+=(const Vector3& _v);
    inline const Vector3& operator-=(const Vector3& _v);
    inline const Vector3& operator*=(const Vector3& _v);
    inline const Vector3& operator/=(const Vector3& _v);
    inline const Vector3& operator*=(float32 f);
    inline const Vector3& operator/=(float32 f);
    inline Vector3 operator-() const;

    //! Comparison operators
    inline bool operator==(const Vector3& _v) const;
    inline bool operator!=(const Vector3& _v) const;
};

//! operators
inline Vector3 operator-(const Vector3& _v1, const Vector3& _v2);
inline Vector3 operator+(const Vector3& _v1, const Vector3& _v2);
inline Vector3 operator*(const Vector3& _v1, const Vector3& _v2);
inline Vector3 operator/(const Vector3& _v1, const Vector3& _v2);

//! with scalar
inline Vector3 operator+(const Vector3& _v, float32 _f);
inline Vector3 operator+(float32 _f, const Vector3& v);

inline Vector3 operator-(const Vector3& _v, float32 _f);
inline Vector3 operator-(float32 _f, const Vector3& v);

inline Vector3 operator*(const Vector3& _v, float32 _f);
inline Vector3 operator*(float32 _f, const Vector3& v);

inline Vector3 operator/(const Vector3& _v, float32 _f);
inline Vector3 operator/(float32 _f, const Vector3& v);

//! additional
inline Vector3 Normalize(const Vector3& v);
inline Vector3 CrossProduct(const Vector3& v1, const Vector3& v2);
inline float32 DotProduct(const Vector3& v1, const Vector3& v2);
inline Vector3 Lerp(const Vector3& _v1, const Vector3& _v2, float32 t);
inline Vector3 Reflect(const Vector3& v, const Vector3& n);
inline float32 Distance(const Vector3& v1, const Vector3& v2);
inline Vector3 PerpendicularVector(const Vector3& normal);
inline Vector3 Floor(const Vector3& v);
inline Vector3 Frac(const Vector3& v);
inline Vector3 Fmod(const Vector3& v, const Vector3& m);
inline Vector3 Max(const Vector3& v1, const Vector3& v2);
inline Vector3 Min(const Vector3& v1, const Vector3& v2);

/**	
    \ingroup math
    \brief Vector with 4 coordinates.
 */
class Vector4
{
public:
    enum eAxis
    {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2,
        AXIS_W = 3,
        AXIS_COUNT = 4
    };
    static const Vector4 Zero;

    union {
        struct
        {
            float32 x, y, z, w;
        };
        float32 data[AXIS_COUNT];
    };

    inline Vector4();
    inline Vector4(float32 _x, float32 _y, float32 _z, float32 _w);
    inline Vector4(const float32* _data);
    inline Vector4(const Vector3& xyz, float32 _w);
    explicit inline Vector4(const Vector3& v);
    inline Vector4(const Vector4& v);
    inline Vector4& operator=(const Vector4& _v);
    inline Vector4& operator=(const Vector3& _v);

    //! Get operators
    float32& operator[](eAxis axis);
    float32 operator[](eAxis axis) const;

    //! Set functions
    inline void Set(float32 _x, float32 _y, float32 _z, float32 _w);

    //! Additional functions
    inline Vector4 CrossProduct(const Vector4& _v) const;
    inline float32 DotProduct(const Vector4& _v) const;
    inline void Lerp(const Vector4& _v1, const Vector4& _v2, float32 t);

    //! On functions
    inline float32 SquareLength();
    inline float32 Length();
    inline void Normalize();
    inline void Clamp(float32 min, float32 max);

    //! On operations
    inline const Vector4& operator+=(const Vector4& _v);
    inline const Vector4& operator-=(const Vector4& _v);
    inline const Vector4& operator*=(float32 f);
    inline const Vector4& operator/=(float32 f);
    inline const Vector4& operator*=(const Vector4& _v);
    inline const Vector4& operator/=(const Vector4& _v);
    inline Vector4 operator-() const;

    //! Comparison operators
    inline bool operator==(const Vector4& _v) const;
    inline bool operator!=(const Vector4& _v) const;

    inline Vector3& GetVector3();
    inline const Vector3& GetVector3() const;
};

//! operators
inline Vector4 operator-(const Vector4& _v1, const Vector4& _v2);
inline Vector4 operator+(const Vector4& _v1, const Vector4& _v2);

inline Vector4 operator*(Vector4 _v1, const Vector4& _v2);
inline Vector4 operator/(Vector4 _v1, const Vector4& _v2);

//! with scalar
inline Vector4 operator+(const Vector4& _v, float32 _f);
inline Vector4 operator+(float32 _f, const Vector4& v);

inline Vector4 operator-(const Vector4& _v, float32 _f);
inline Vector4 operator-(float32 _f, const Vector4& v);

inline Vector4 operator*(const Vector4& _v, float32 _f);
inline Vector4 operator*(float32 _f, const Vector4& v);

inline Vector4 operator/(const Vector4& _v, float32 _f);
inline Vector4 operator/(float32 _f, const Vector4& v);

//! additional
inline Vector4 Normalize(const Vector4& v);
inline Vector4 CrossProduct(const Vector4& v1, const Vector4& v2);
inline float32 DotProduct(const Vector4& v1, const Vector4& v2);
inline Vector4 Lerp(const Vector4& _v1, const Vector4& _v2, float32 t);
inline Vector4 Floor(const Vector4& v);
inline Vector4 Frac(const Vector4& v);
inline Vector4 Fmod(const Vector4& v, const Vector4& m);
inline Vector4 Abs(const Vector4& v);
inline Vector4 Max(const Vector4& v1, const Vector4& v2);
inline Vector4 Min(const Vector4& v1, const Vector4& v2);
// Vector2 Implementation

inline Vector2::Vector2()
{
    x = 0;
    y = 0;
}

inline Vector2::Vector2(float32 _x, float32 _y)
{
    x = _x;
    y = _y;
}

inline Vector2::Vector2(const float32* _data)
{
    data[0] = _data[0];
    data[1] = _data[1];
}

inline Vector2::Vector2(const Vector2& _v)
{
    x = _v.x;
    y = _v.y;
}

inline Vector2& Vector2::operator=(const Vector2& _v)
{
    x = _v.x;
    y = _v.y;
    return *this;
}

//! set functions
inline void Vector2::Set(float32 _x, float32 _y)
{
    x = _x;
    y = _y;
}

inline void Vector2::Lerp(const Vector2& _v1, const Vector2& _v2, float32 t)
{
    x = _v1.x * (1.0f - t) + _v2.x * t;
    y = _v1.y * (1.0f - t) + _v2.y * t;
}

// Get operators
inline float32& Vector2::operator[](eAxis axis)
{
    return data[axis];
}

inline const float32 Vector2::operator[](eAxis axis) const
{
    return data[axis];
}

// On operators

inline const Vector2& Vector2::operator+=(const Vector2& _v)
{
    x += _v.x;
    y += _v.y;
    return *this;
}

inline const Vector2& Vector2::operator-=(const Vector2& _v)
{
    x -= _v.x;
    y -= _v.y;
    return *this;
}

inline const Vector2& Vector2::operator*=(const Vector2& _v)
{
    x *= _v.x;
    y *= _v.y;
    return *this;
}

inline const Vector2& Vector2::operator/=(const Vector2& _v)
{
    x /= _v.x;
    y /= _v.y;
    return *this;
}

inline const Vector2& Vector2::operator*=(float32 _f)
{
    x *= _f;
    y *= _f;
    return *this;
}
inline const Vector2& Vector2::operator/=(float32 _f)
{
    x /= _f;
    y /= _f;
    return *this;
}

inline Vector2 Vector2::operator-() const
{
    return Vector2(-x, -y);
}

//! Comparison operators
inline bool Vector2::operator==(const Vector2& _v) const
{
    return (Memcmp(data, _v.data, sizeof(Vector2)) == 0);
}
inline bool Vector2::operator!=(const Vector2& _v) const
{
    return (Memcmp(data, _v.data, sizeof(Vector2)) != 0);
}

//! On functions

inline float32 Vector2::SquareLength() const
{
    return x * x + y * y;
}

inline float32 Vector2::Length() const
{
    return std::sqrt(SquareLength());
}

inline void Vector2::Normalize()
{
    float32 len = Length();
    x /= len;
    y /= len;
}

//! Additional functions
inline float32 Vector2::DotProduct(const Vector2& _v2) const
{
    return (x * _v2.x + y * _v2.y);
}
inline float32 Vector2::CrossProduct(const Vector2& b) const
{
    return (this->x * b.y - this->y * b.x);
}

inline float32 Vector2::Angle() const
{
    float angle = (std::abs(x) > std::abs(y)) ? std::acos(std::abs(x)) : std::asin(std::abs(y));

    if (x >= 0 && y >= 0)
        return angle; // I
    if (x <= 0 && y >= 0)
        return PI - angle; // II
    if (x <= 0 && y <= 0)
        return PI + angle; // III

    return 2.0f * PI - angle; // IV
}

inline Vector2 operator-(const Vector2& _v1, const Vector2& _v2)
{
    return Vector2(_v1.x - _v2.x, _v1.y - _v2.y);
}

inline Vector2 operator+(const Vector2& _v1, const Vector2& _v2)
{
    return Vector2(_v1.x + _v2.x, _v1.y + _v2.y);
}

inline Vector2 operator*(const Vector2& _v1, const Vector2& _v2)
{
    return Vector2(_v1.x * _v2.x, _v1.y * _v2.y);
}

inline Vector2 operator/(const Vector2& _v1, const Vector2& _v2)
{
    return Vector2(_v1.x / _v2.x, _v1.y / _v2.y);
}

//! With scalars
inline Vector2 operator+(const Vector2& _v, float32 _f)
{
    return Vector2(_v.x + _f, _v.y + _f);
}

inline Vector2 operator+(float32 _f, const Vector2& _v)
{
    return Vector2(_v.x + _f, _v.y + _f);
}

inline Vector2 operator-(const Vector2& _v, float32 _f)
{
    return Vector2(_v.x - _f, _v.y - _f);
}

inline Vector2 operator-(float32 _f, const Vector2& _v)
{
    return Vector2(_f - _v.x, _f - _v.y);
}

inline Vector2 operator*(const Vector2& _v, float32 _f)
{
    return Vector2(_v.x * _f, _v.y * _f);
}

inline Vector2 operator*(float32 _f, const Vector2& _v)
{
    return Vector2(_v.x * _f, _v.y * _f);
}

inline Vector2 operator/(const Vector2& _v, float32 _f)
{
    return Vector2(_v.x / _f, _v.y / _f);
}

inline Vector2 operator/(float32 _f, const Vector2& _v)
{
    return Vector2(_f / _v.x, _f / _v.y);
}

//! functions
inline float32 DotProduct(const Vector2& _v1, const Vector2& _v2)
{
    return _v1.DotProduct(_v2);
}
inline Vector2 Normalize(const Vector2& _v)
{
    Vector2 v(_v);
    v.Normalize();
    return v;
}

inline float32 CrossProduct(const Vector2& a, const Vector2& b)
{
    return (a.x * b.y - a.y * b.x);
}

inline Vector2 Reflect(const Vector2& v, const Vector2& n)
{
    Vector2 r = v - (2.0f * DotProduct(v, n)) * n;
    return r;
}

// Vector3 Implementation
inline Vector3::Vector3()
{
    x = y = z = 0;
}

inline Vector3::Vector3(float32 _x, float32 _y, float32 _z)
{
    x = _x;
    y = _y;
    z = _z;
}

inline Vector3::Vector3(const float32* _data)
{
    data[0] = _data[0];
    data[1] = _data[1];
    data[2] = _data[2];
}

inline Vector3::Vector3(const Vector2& v)
{
    x = v.x;
    y = v.y;
    z = 0.0f;
}

inline Vector3::Vector3(const Vector3& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
}

inline Vector3::Vector3(const Vector2& v, float _z)
{
    x = v.x;
    y = v.y;
    z = _z;
}

inline Vector3::Vector3(const Vector4& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
}

inline Vector3& Vector3::operator=(const Vector3& _v)
{
    x = _v.x;
    y = _v.y;
    z = _v.z;
    return (*this);
}
inline Vector3& Vector3::operator=(const Vector2& _v)
{
    x = _v.x;
    y = _v.y;
    z = 0.0f;
    return (*this);
}

// Get operators
inline float32& Vector3::operator[](eAxis axis)
{
    return data[axis];
}

inline float32 Vector3::operator[](eAxis axis) const
{
    return data[axis];
}

//! set functions
inline void Vector3::Set(float32 _x, float32 _y, float32 _z)
{
    x = _x;
    y = _y;
    z = _z;
}

//! Additional functions
inline float32 Vector3::DotProduct(const Vector3& _v) const
{
    return (x * _v.x) + (y * _v.y) + (z * _v.z);
}

inline Vector3 Vector3::CrossProduct(const Vector3& v) const
{
    return Vector3(y * v.z - v.y * z, z * v.x - x * v.z, x * v.y - y * v.x);
}

inline void Vector3::CrossProduct(const Vector3& v1, const Vector3& v2)
{
    // TODO: This is wrong if &v1 or &v2 is this.
    x = (v1.y * v2.z) - (v1.z * v2.y);
    y = (v1.z * v2.x) - (v1.x * v2.z);
    z = (v1.x * v2.y) - (v1.y * v2.x);
}

//! On functions
inline float32 Vector3::SquareLength() const
{
    return x * x + y * y + z * z;
}
inline float32 Vector3::Length() const
{
    return std::sqrt(SquareLength());
}
inline float32 Vector3::Normalize()
{
    const float32 len = Length();
    x /= len;
    y /= len;
    z /= len;
    return len;
}

inline void Vector3::Lerp(const Vector3& _v1, const Vector3& _v2, float32 t)
{
    x = _v1.x * (1.0f - t) + _v2.x * t;
    y = _v1.y * (1.0f - t) + _v2.y * t;
    z = _v1.z * (1.0f - t) + _v2.z * t;
}

inline void Vector3::Clamp(float32 min, float32 max)
{
    if (x < min)
        x = min;
    if (y < min)
        y = min;
    if (z < min)
        z = min;

    if (x > max)
        x = max;
    if (y > max)
        y = max;
    if (z > max)
        z = max;
}

//! On operations
inline const Vector3& Vector3::operator+=(const Vector3& _v)
{
    x += _v.x;
    y += _v.y;
    z += _v.z;
    return *this;
}

inline const Vector3& Vector3::operator-=(const Vector3& _v)
{
    x -= _v.x;
    y -= _v.y;
    z -= _v.z;
    return *this;
}

inline const Vector3& Vector3::operator*=(const Vector3& _v)
{
    x *= _v.x;
    y *= _v.y;
    z *= _v.z;
    return *this;
}

inline const Vector3& Vector3::operator/=(const Vector3& _v)
{
    x /= _v.x;
    y /= _v.y;
    z /= _v.z;
    return *this;
}

inline const Vector3& Vector3::operator*=(float32 f)
{
    x *= f;
    y *= f;
    z *= f;
    return *this;
}
inline const Vector3& Vector3::operator/=(float32 f)
{
    x /= f;
    y /= f;
    z /= f;
    return *this;
}
inline Vector3 Vector3::operator-() const
{
    return Vector3(-x, -y, -z);
}

//! Comparison operators
inline bool Vector3::operator==(const Vector3& _v) const
{
    return (Memcmp(data, _v.data, sizeof(Vector3)) == 0);
}
inline bool Vector3::operator!=(const Vector3& _v) const
{
    return (Memcmp(data, _v.data, sizeof(Vector3)) != 0);
}

//! operators
inline Vector3 operator-(const Vector3& _v1, const Vector3& _v2)
{
    return Vector3(_v1.x - _v2.x, _v1.y - _v2.y, _v1.z - _v2.z);
}
inline Vector3 operator+(const Vector3& _v1, const Vector3& _v2)
{
    return Vector3(_v1.x + _v2.x, _v1.y + _v2.y, _v1.z + _v2.z);
}

inline Vector3 operator*(const Vector3& _v1, const Vector3& _v2)
{
    return Vector3(_v1.x * _v2.x, _v1.y * _v2.y, _v1.z * _v2.z);
}
inline Vector3 operator/(const Vector3& _v1, const Vector3& _v2)
{
    return Vector3(_v1.x / _v2.x, _v1.y / _v2.y, _v1.z / _v2.z);
}

//! with scalar
inline Vector3 operator+(const Vector3& _v, float32 _f)
{
    return Vector3(_v.x + _f, _v.y + _f, _v.z + _f);
}

inline Vector3 operator+(float32 _f, const Vector3& _v)
{
    return Vector3(_v.x + _f, _v.y + _f, _v.z + _f);
}

inline Vector3 operator-(const Vector3& _v, float32 _f)
{
    return Vector3(_v.x - _f, _v.y - _f, _v.z - _f);
}
inline Vector3 operator-(float32 _f, const Vector3& _v)
{
    return Vector3(_f - _v.x, _f - _v.y, _f - _v.z);
}

inline Vector3 operator*(const Vector3& _v, float32 _f)
{
    return Vector3(_v.x * _f, _v.y * _f, _v.z * _f);
}

inline Vector3 operator*(float32 _f, const Vector3& _v)
{
    return Vector3(_f * _v.x, _f * _v.y, _f * _v.z);
}

inline Vector3 operator/(const Vector3& _v, float32 _f)
{
    return Vector3(_v.x / _f, _v.y / _f, _v.z / _f);
}

inline Vector3 operator/(float32 _f, const Vector3& _v)
{
    return Vector3(_f / _v.x, _f / _v.y, _f / _v.z);
}

//! additional
inline Vector3 CrossProduct(const Vector3& v1, const Vector3& v2)
{
    return v1.CrossProduct(v2);
}
inline float32 DotProduct(const Vector3& v1, const Vector3& v2)
{
    return v1.DotProduct(v2);
}
inline Vector3 Lerp(const Vector3& _v1, const Vector3& _v2, float32 t)
{
    Vector3 v;
    v.Lerp(_v1, _v2, t);
    return v;
}

inline Vector3 Normalize(const Vector3& v)
{
    Vector3 res(v);
    res.Normalize();
    return res;
}

inline Vector3 Reflect(const Vector3& v, const Vector3& n)
{
    Vector3 r = v - (2 * DotProduct(v, n)) * n;
    return r;
}

inline float32 Distance(const Vector3& v1, const Vector3& v2)
{
    float32 dx = v1.x - v2.x;
    float32 dy = v1.y - v2.y;
    float32 dz = v1.z - v2.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

inline Vector3 PerpendicularVector(const Vector3& normal)
{
    Vector3 componentsLength(normal.x * normal.x, normal.y * normal.y, normal.z * normal.z);

    if (componentsLength.x > 0.5f)
    {
        float32 scaleFactor = std::sqrt(componentsLength.z + componentsLength.x);
        return Vector3(normal.z / scaleFactor, 0.0f, -normal.x / scaleFactor);
    }
    else if (componentsLength.y > 0.5f)
    {
        float32 scaleFactor = std::sqrt(componentsLength.y + componentsLength.x);
        return Vector3(-normal.y / scaleFactor, normal.x / scaleFactor, 0.0f);
    }

    float32 scaleFactor = std::sqrt(componentsLength.z + componentsLength.y);
    return Vector3(0.0f, -normal.z / scaleFactor, normal.y / scaleFactor);
}

inline Vector3 Floor(const Vector3& v)
{
    return Vector3
    (
    std::floor(v.x),
    std::floor(v.y),
    std::floor(v.z)
    );
}

inline Vector3 Frac(const Vector3& v)
{
    return Vector3
    (
    std::fmod(v.x, 1.0f),
    std::fmod(v.y, 1.0f),
    std::fmod(v.z, 1.0f)
    );
}

inline Vector3 Fmod(const Vector3& v, const Vector3& m)
{
    return Vector3
    (
    std::fmod(v.x, m.x),
    std::fmod(v.y, m.y),
    std::fmod(v.z, m.z)
    );
}

inline Vector3 Max(const Vector3& v1, const Vector3& v2)
{
    return Vector3
    (
    std::max(v1.x, v2.x),
    std::max(v1.y, v2.y),
    std::max(v1.z, v2.z)
    );
}

inline Vector3 Min(const Vector3& v1, const Vector3& v2)
{
    return Vector3
    (
    std::min(v1.x, v2.x),
    std::min(v1.y, v2.y),
    std::min(v1.z, v2.z)
    );
}

// Vector4 implementation
inline Vector4::Vector4()
{
    x = y = z = w = 0;
}

inline Vector4::Vector4(float32 _x, float32 _y, float32 _z, float32 _w)
{
    x = _x;
    y = _y;
    z = _z;
    w = _w;
}

inline Vector4::Vector4(const float32* _data)
{
    data[0] = _data[0];
    data[1] = _data[1];
    data[2] = _data[2];
    data[3] = _data[3];
}

inline Vector4::Vector4(const Vector3& xyz, float32 _w)
    : x(xyz.x)
    , y(xyz.y)
    , z(xyz.z)
    , w(_w)
{
}

inline Vector4::Vector4(const Vector3& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = 1.0f;
}

inline Vector4::Vector4(const Vector4& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
}

inline Vector4& Vector4::operator=(const Vector4& _v)
{
    x = _v.x;
    y = _v.y;
    z = _v.z;
    w = _v.w;
    return (*this);
}

inline Vector4& Vector4::operator=(const Vector3& _v)
{
    x = _v.x;
    y = _v.y;
    z = _v.z;
    w = 1.0f;
    return (*this);
}

// Get operators
inline float32& Vector4::operator[](eAxis axis)
{
    return data[axis];
}

inline float32 Vector4::operator[](eAxis axis) const
{
    return data[axis];
}

//! set functions
inline void Vector4::Set(float32 _x, float32 _y, float32 _z, float32 _w)
{
    x = _x;
    y = _y;
    z = _z;
    w = _w;
}

//! Additional functions
inline float32 Vector4::DotProduct(const Vector4& _v) const
{
    return (x * _v.x) + (y * _v.y) + (z * _v.z);
}

inline Vector4 Vector4::CrossProduct(const Vector4& v) const
{
    return Vector4(y * v.z - v.y * z, z * v.x - x * v.z, x * v.y - y * v.x, 1.0f);
}

//! On functions
inline float32 Vector4::SquareLength()
{
    return x * x + y * y + z * z + w * w;
}
inline float32 Vector4::Length()
{
    return std::sqrt(SquareLength());
}
inline void Vector4::Normalize()
{
    float32 len = Length();
    x /= len;
    y /= len;
    z /= len;
    w /= len;
}

inline void Vector4::Lerp(const Vector4& _v1, const Vector4& _v2, float32 t)
{
    x = _v1.x * (1.0f - t) + _v2.x * t;
    y = _v1.y * (1.0f - t) + _v2.y * t;
    z = _v1.z * (1.0f - t) + _v2.z * t;
    w = _v1.w * (1.0f - t) + _v2.w * t;
}

inline void Vector4::Clamp(float32 min, float32 max)
{
    if (x < min)
        x = min;
    if (y < min)
        y = min;
    if (z < min)
        z = min;
    if (w < min)
        w = min;

    if (x > max)
        x = max;
    if (y > max)
        y = max;
    if (z > max)
        z = max;
    if (w > max)
        w = max;
}

//! On operations
inline const Vector4& Vector4::operator+=(const Vector4& _v)
{
    x += _v.x;
    y += _v.y;
    z += _v.z;
    w += _v.w;
    return *this;
}

inline const Vector4& Vector4::operator-=(const Vector4& _v)
{
    x -= _v.x;
    y -= _v.y;
    z -= _v.z;
    w -= _v.w;
    return *this;
}

inline const Vector4& Vector4::operator*=(float32 f)
{
    x *= f;
    y *= f;
    z *= f;
    w *= f;
    return *this;
}
inline const Vector4& Vector4::operator/=(float32 f)
{
    x /= f;
    y /= f;
    z /= f;
    w /= f;
    return *this;
}

inline const Vector4& Vector4::operator*=(const Vector4& _v)
{
    x *= _v.x;
    y *= _v.y;
    z *= _v.z;
    w *= _v.w;
    return *this;
}

inline const Vector4& Vector4::operator/=(const Vector4& _v)
{
    x *= _v.x;
    y *= _v.y;
    z *= _v.z;
    w *= _v.w;
    return *this;
}

inline Vector4 Vector4::operator-() const
{
    return Vector4(-x, -y, -z, -w);
}

//! Comparison operators
inline bool Vector4::operator==(const Vector4& _v) const
{
    return (Memcmp(data, _v.data, sizeof(Vector4)) == 0);
}
inline bool Vector4::operator!=(const Vector4& _v) const
{
    return (Memcmp(data, _v.data, sizeof(Vector4)) != 0);
}

inline const Vector3& Vector4::GetVector3() const
{
    return *(reinterpret_cast<const Vector3*>(data));
}

inline Vector3& Vector4::GetVector3()
{
    return *(reinterpret_cast<Vector3*>(data));
}

//! operators
inline Vector4 operator-(const Vector4& _v1, const Vector4& _v2)
{
    return Vector4(_v1.x - _v2.x, _v1.y - _v2.y, _v1.z - _v2.z, _v1.w - _v2.w);
}
inline Vector4 operator+(const Vector4& _v1, const Vector4& _v2)
{
    return Vector4(_v1.x + _v2.x, _v1.y + _v2.y, _v1.z + _v2.z, _v1.w + _v2.w);
}

inline Vector4 operator*(Vector4 _v1, const Vector4& _v2)
{
    _v1 *= _v2;
    return _v1;
}

inline Vector4 operator/(Vector4 _v1, const Vector4& _v2)
{
    _v1 /= _v2;
    return _v1;
}

//! with scalar
inline Vector4 operator+(const Vector4& _v, float32 _f)
{
    return Vector4(_v.x + _f, _v.y + _f, _v.z + _f, _v.w + _f);
}

inline Vector4 operator+(float32 _f, const Vector4& _v)
{
    return Vector4(_v.x + _f, _v.y + _f, _v.z + _f, _v.w + _f);
}

inline Vector4 operator-(const Vector4& _v, float32 _f)
{
    return Vector4(_v.x - _f, _v.y - _f, _v.z - _f, _v.w - _f);
}
inline Vector4 operator-(float32 _f, const Vector4& _v)
{
    return Vector4(_f - _v.x, _f - _v.y, _f - _v.z, _f - _v.w);
}

inline Vector4 operator*(const Vector4& _v, float32 _f)
{
    return Vector4(_v.x * _f, _v.y * _f, _v.z * _f, _v.w * _f);
}

inline Vector4 operator*(float32 _f, const Vector4& _v)
{
    return Vector4(_f * _v.x, _f * _v.y, _f * _v.z, _f * _v.w);
}

inline Vector4 operator/(const Vector4& _v, float32 _f)
{
    return Vector4(_v.x / _f, _v.y / _f, _v.z / _f, _v.w / _f);
}

inline Vector4 operator/(float32 _f, const Vector4& _v)
{
    return Vector4(_f / _v.x, _f / _v.y, _f / _v.z, _f / _v.w);
}

//! additional
inline Vector4 CrossProduct(const Vector4& v1, const Vector4& v2)
{
    return v1.CrossProduct(v2);
}
inline float32 DotProduct(const Vector4& v1, const Vector4& v2)
{
    return v1.DotProduct(v2);
}
inline Vector4 Lerp(const Vector4& _v1, const Vector4& _v2, float32 t)
{
    Vector4 v;
    v.Lerp(_v1, _v2, t);
    return v;
}

inline Vector4 Floor(const Vector4& v)
{
    return
    {
      std::floor(v.x),
      std::floor(v.y),
      std::floor(v.z),
      std::floor(v.w)
    };
}

inline Vector4 Frac(const Vector4& v)
{
    return Vector4
    (
    std::fmod(v.x, 1.0f),
    std::fmod(v.y, 1.0f),
    std::fmod(v.z, 1.0f),
    std::fmod(v.w, 1.0f)
    );
}

inline Vector4 Fmod(const Vector4& v, const Vector4& m)
{
    return Vector4
    (
    std::fmod(v.x, m.x),
    std::fmod(v.y, m.y),
    std::fmod(v.z, m.z),
    std::fmod(v.w, m.w)
    );
}

inline Vector4 Abs(const Vector4& v)
{
    return Vector4
    (
    std::abs(v.x),
    std::abs(v.y),
    std::abs(v.z),
    std::abs(v.w)
    );
}

inline Vector4 Max(const Vector4& v1, const Vector4& v2)
{
    return Vector4
    (
    std::max(v1.x, v2.x),
    std::max(v1.y, v2.y),
    std::max(v1.z, v2.z),
    std::max(v1.w, v2.w)
    );
}

inline Vector4 Min(const Vector4& v1, const Vector4& v2)
{
    return Vector4
    (
    std::min(v1.x, v2.x),
    std::min(v1.y, v2.y),
    std::min(v1.z, v2.z),
    std::min(v1.w, v2.w)
    );
}

inline Vector4 Normalize(const Vector4& v)
{
    Vector4 res(v);
    res.Normalize();
    return res;
}

template <>
bool AnyCompare<Vector2>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<Vector2>;

template <>
bool AnyCompare<Vector3>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<Vector3>;

template <>
bool AnyCompare<Vector4>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<Vector4>;
};
