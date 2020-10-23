#pragma once
#include <cmath>

#include "Vector.h"
#include "Math/Math2D.h"

#include "Base/Any.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Rect in 2D space. This class is used for all rectangles along all SDK subsystems.
 */
struct Rect
{
    float32 x;
    float32 y;
    float32 dx;
    float32 dy;

    inline Rect();
    inline Rect(float32 _x, float32 _y, float32 _dx, float32 _dy);
    inline Rect(const Rect& rect);
    inline Rect(const Vector2& point, const Vector2& size);
    inline Rect(const Size2f& size);

    inline bool PointInside(const Vector2& point) const;
    inline Rect Intersection(const Rect& rect) const;
    inline bool RectIntersects(const Rect& rect) const;
    inline bool RectContains(const Rect& rect) const;
    inline void ClampToRect(Vector2& point) const;
    inline void ClampToRect(Rect& rect) const;
    inline Rect Combine(const Rect& rect) const;

    inline Vector2 GetCenter() const;
    inline Vector2 GetPosition() const;
    inline Vector2 GetSize() const;

    inline void SetCenter(const Vector2& center);
    inline void SetPosition(const Vector2& position);
    inline void SetSize(const Vector2& size);

    inline Rect& operator=(const Rect& r);

    inline bool operator==(const Rect& _s) const;
    inline bool operator!=(const Rect& _s) const;

    inline Rect& operator+=(const Vector2& point);
    inline Rect& operator-=(const Vector2& point);

    inline Rect operator+(const Vector2& Point) const;
    inline Rect operator-(const Vector2& Point) const;
};

inline Rect::Rect()
{
    x = y = dx = dy = 0;
};

inline Rect::Rect(float32 _x, float32 _y, float32 _dx, float32 _dy)
{
    x = _x;
    y = _y;
    dx = _dx;
    dy = _dy;
};

inline Rect::Rect(const Rect& rect)
{
    x = rect.x;
    y = rect.y;
    dx = rect.dx;
    dy = rect.dy;
};

inline Rect::Rect(const Vector2& point, const Vector2& size)
{
    x = point.x;
    y = point.y;
    dx = size.x;
    dy = size.y;
}

inline Rect::Rect(const Size2f& size)
{
    x = 0;
    y = 0;
    dx = size.dx;
    dy = size.dy;
}

inline bool Rect::PointInside(const Vector2& point) const
{
    if ((point.x >= x) && (point.x < x + dx)
        && (point.y >= y) && (point.y < y + dy))
        return true;
    return false;
}

inline bool Rect::RectIntersects(const Rect& rect) const
{
    float32 top1 = y;
    float32 top2 = rect.y;
    float32 bottom1 = y + dy;
    float32 bottom2 = rect.y + rect.dy;
    if (top1 > bottom1 || top2 > bottom2 || bottom1 < top2 || top1 > bottom2)
    {
        return false;
    }
    float32 left1 = x;
    float32 left2 = rect.x;
    float32 right1 = x + dx;
    float32 right2 = rect.x + rect.dx;
    if (left1 > right1 || left2 > right2 || right1 < left2 || left1 > right2)
    {
        return false;
    }

    return true;
}

//realization from Qt QRect.cpp: bool QRectF::contains(const QRectF &r) const
inline bool Rect::RectContains(const Rect& rect) const
{
    float32 top1 = y;
    float32 bottom1 = y + dy;
    float32 top2 = rect.y;
    float32 bottom2 = rect.y + rect.dy;

    if (top1 > bottom1 || top2 > bottom2 || top2 < top1 || bottom2 > bottom1)
    {
        return false;
    }

    float32 left1 = x;
    float32 right1 = x + dx;
    float32 left2 = rect.x;
    float32 right2 = rect.x + rect.dx;

    if (left1 > right1 || left2 > right2 || left2 < left1 || right2 > right1)
    {
        return false;
    }

    return true;
}

inline Rect Rect::Intersection(const Rect& rect) const
{
    float32 nx = Max(x, rect.x);
    float32 ny = Max(y, rect.y);
    float32 ndx = Min((dx + x) - nx, (rect.dx + rect.x) - nx);
    float32 ndy = Min((dy + y) - ny, (rect.dy + rect.y) - ny);
    if (ndx <= 0 || ndy <= 0)
    {
        ndx = 0;
        ndy = 0;
    }

    return Rect(nx, ny, ndx, ndy);
}

inline void Rect::ClampToRect(Vector2& point) const
{
    if (point.x < x)
        point.x = x;
    if (point.y < y)
        point.y = y;
    if (point.x > x + dx)
        point.x = x + dx;
    if (point.y > y + dy)
        point.y = y + dy;
}

inline void Rect::ClampToRect(Rect& rect) const
{
    Vector2 topLeft(rect.x, rect.y);
    Vector2 bottomRight = topLeft + Vector2(rect.dx, rect.dy);

    ClampToRect(topLeft);
    ClampToRect(bottomRight);

    rect = Rect(topLeft, bottomRight - topLeft);
}

inline Rect Rect::Combine(const Rect& rect) const
{
    Vector2 topLeft(x, y);
    Vector2 bottomRight(x + dx, y + dy);
    topLeft.x = Min(topLeft.x, rect.x);
    topLeft.y = Min(topLeft.y, rect.y);
    bottomRight.x = Max(bottomRight.x, rect.x + rect.dx);
    bottomRight.y = Max(bottomRight.y, rect.y + rect.dy);
    return Rect(topLeft, bottomRight - topLeft);
}

inline Vector2 Rect::GetCenter() const
{
    return Vector2(x + (dx * .5f), y + (dy * .5f));
}

inline Vector2 Rect::GetPosition() const
{
    return Vector2(x, y);
}

inline Vector2 Rect::GetSize() const
{
    return Vector2(dx, dy);
}

inline void Rect::SetCenter(const Vector2& center)
{
    x = center.x - dx * 0.5f;
    y = center.y - dy * 0.5f;
}

inline void Rect::SetPosition(const Vector2& position)
{
    x = position.x;
    y = position.y;
}

inline void Rect::SetSize(const Vector2& size)
{
    dx = size.dx;
    dy = size.dy;
}

inline Rect& Rect::operator=(const Rect& rect)
{
    x = rect.x;
    y = rect.y;
    dx = rect.dx;
    dy = rect.dy;
    return *this;
}

inline bool Rect::operator==(const Rect& _r) const
{
    return (Memcmp(&x, &_r.x, sizeof(float32)) == 0) &&
    (Memcmp(&y, &_r.y, sizeof(float32)) == 0) &&
    (Memcmp(&dx, &_r.dx, sizeof(float32)) == 0) &&
    (Memcmp(&dy, &_r.dy, sizeof(float32)) == 0);
}

inline bool Rect::operator!=(const Rect& _r) const
{
    return (!Rect::operator==(_r));
}

inline Rect& Rect::operator+=(const Vector2& pt)
{
    x += pt.x;
    y += pt.y;
    return *this;
}

inline Rect& Rect::operator-=(const Vector2& pt)
{
    x -= pt.x;
    y -= pt.y;
    return *this;
}

inline Rect Rect::operator+(const Vector2& pt) const
{
    return Rect(x + pt.x, y + pt.y, dx, dy);
}

inline Rect Rect::operator-(const Vector2& pt) const
{
    return Rect(x - pt.x, y - pt.y, dx, dy);
}

template <>
bool AnyCompare<Rect>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<Rect>;

}; // end of namespace DAVA
