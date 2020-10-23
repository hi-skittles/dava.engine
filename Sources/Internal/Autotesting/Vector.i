%module Vector
%{
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"

#include "Math/Vector.h"
%}

%import Base/BaseTypes.h
%import Base/BaseMath.h

namespace DAVA
{

class Vector2
{
public:
	float32 x, y;
	
	//! Basic
	inline Vector2();
	inline Vector2(float32 _x, float32 _y);
	inline Vector2(float32 *_data);
	inline Vector2 & operator =(const Vector2 & _v);

	//! Set functions
	inline void	Set(float32 _x, float32 _y);
	
	//! Additional functions
	inline void	Lerp(const Vector2 & _v1, const Vector2 & _v2, float32 t);
	
	//! On functions
	inline float32 SquareLength() const;
	inline float32 Length() const;
	inline void Normalize();
	
	//! Additional functions
	inline float32 DotProduct(const Vector2 & _v2) const;
	inline float32 CrossProduct(const Vector2 & b) const;
	inline float32 Angle() const; // Need to normalize vector before use function

    inline bool IsZero() const { return x == 0.f && y == 0.f; }
    inline void SetZero() { x = y = 0.f; } // = 0

	//! On operations
	inline const Vector2 & operator += (const Vector2 & _v);
	inline const Vector2 & operator -= (const Vector2 & _v);
	inline const Vector2 & operator *= (float32 f);
	inline const Vector2 & operator /= (float32 f);
	inline Vector2 operator -() const;

	//! Comparison operators
	inline bool operator == (const Vector2 & _v) const;
	inline bool operator != (const Vector2 & _v) const;
	
};

//! operators
inline Vector2 operator - (const Vector2 & _v1, const Vector2 & _v2);
inline Vector2 operator + (const Vector2 & _v1, const Vector2 & _v2);

//! with scalar
inline Vector2 operator + (const Vector2 & _v, float32 _f);
inline Vector2 operator + (float32 _f, const Vector2 & v);

inline Vector2 operator - (const Vector2 & _v, float32 _f);
inline Vector2 operator - (float32 _f, const Vector2 & v);

inline Vector2 operator * (const Vector2 & _v, float32 _f);
inline Vector2 operator * (float32 _f, const Vector2 & v);

inline Vector2 operator / (const Vector2 & _v, float32 _f);
inline Vector2 operator / (float32 _f, const Vector2 & v);


//! functions
inline float32 DotProduct(const Vector2 & _v1, const Vector2 & _v2);
inline Vector2 Normalize(const Vector2 & _v);
inline float32 CrossProduct(const Vector2 & a, const Vector2 & b);
inline Vector2 Reflect(const Vector2 & v, const Vector2 & n);

class Vector3
{
public:
	float32 x, y, z;

	inline Vector3();
	inline Vector3(float32 _x, float32 _y, float32 _z);
	inline Vector3(float32 *_data);
	explicit inline Vector3(const Vector2 & v);
	inline Vector3 & operator =(const Vector3 & _v);
	inline Vector3 & operator =(const Vector2 & _v);
	
	//! Set functions
	inline void	Set(float32 _x, float32 _y, float32 _z);
	
	//! Additional functions
	inline Vector3	CrossProduct(const Vector3 & _v) const;
	inline void     CrossProduct(const Vector3& v1, const Vector3& v2);
	inline float32	DotProduct(const Vector3 & _v) const;
	inline void		Lerp(const Vector3 & _v1, const Vector3 & _v2, float32 t);

    inline float32 Yaw() const { return atan2f(x, y); }
    inline float32 Pitch() const { return -atan2f(z, sqrtf(x*x + y*y)); }

    inline bool IsZero() const { return x == 0.f && y == 0.f && z == 0.f; }
    inline void Zerofy() { x = y = z = 0.f; } // = 0

	//! On functions
	inline float32 SquareLength()  const;
	inline float32 Length() const;
    inline float32 Normalize();
	inline void Clamp(float32 min, float32 max);

	//! On operations
	inline const Vector3 & operator += (const Vector3 & _v);
	inline const Vector3 & operator -= (const Vector3 & _v);
	inline const Vector3 & operator *= (float32 f);
	inline const Vector3 & operator /= (float32 f);
	inline Vector3 operator -() const;

	//! Comparison operators
	inline bool operator == (const Vector3 & _v) const;
	inline bool operator != (const Vector3 & _v) const;	

};

//! operators
inline Vector3 operator - (const Vector3 & _v1, const Vector3 & _v2);
inline Vector3 operator + (const Vector3 & _v1, const Vector3 & _v2);

//! with scalar
inline Vector3 operator + (const Vector3 & _v, float32 _f);
inline Vector3 operator + (float32 _f, const Vector3 & v);

inline Vector3 operator - (const Vector3 & _v, float32 _f);
inline Vector3 operator - (float32 _f, const Vector3 & v);

inline Vector3 operator * (const Vector3 & _v, float32 _f);
inline Vector3 operator * (float32 _f, const Vector3 & v);

inline Vector3 operator / (const Vector3 & _v, float32 _f);
inline Vector3 operator / (float32 _f, const Vector3 & v);


//! additional
inline Vector3 Normalize(const Vector3 & v);
inline Vector3 CrossProduct(const Vector3 & v1, const Vector3 & v2);
inline float32 DotProduct(const Vector3 & v1, const Vector3 & v2);
inline Vector3 Lerp(const Vector3 & _v1, const Vector3 & _v2, float32 t);
inline Vector3 Reflect(const Vector3 & v, const Vector3 & n);

};
