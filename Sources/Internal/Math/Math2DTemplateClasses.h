#pragma once

#include <cmath>
#include "Math/Matrix2.h"
#include "Math/Matrix3.h"

namespace DAVA
{
struct Matrix2;

//! Base class template for 2D points
template <class TYPE>
struct Point2Base
{
    union
    {
        struct
        {
            TYPE x, y;
        };
        TYPE data[2];
    };

    inline Point2Base();
    inline Point2Base(TYPE _x, TYPE _y);
    inline Point2Base(const Point2Base<TYPE>& Point);

    inline Point2Base<TYPE>& operator+=(const Point2Base<TYPE>& Point);
    inline Point2Base<TYPE>& operator-=(const Point2Base<TYPE>& Point);

    inline Point2Base<TYPE> operator+(const Point2Base<TYPE>& Point) const;
    inline Point2Base<TYPE> operator-(const Point2Base<TYPE>& Point) const;

    inline Point2Base<TYPE> operator*(const TYPE t) const;
    inline Point2Base<TYPE>& operator*=(const TYPE t);

    inline Point2Base<TYPE> operator*(const Matrix2& Matrix) const;
    inline Point2Base<TYPE>& operator*=(const Matrix2& Matrix);

    inline Point2Base<TYPE> operator*(const Matrix3& Matrix) const;
    inline Point2Base<TYPE>& operator*=(const Matrix3& Matrix);

    inline bool operator==(const Point2Base<TYPE>&) const;
    inline bool operator!=(const Point2Base<TYPE>&) const;

    // Vector2 operations (for using 2d point as 2d vector)
    inline TYPE Length();
    inline void Normalize();
};

template <class TYPE>
inline TYPE LineLength(Point2Base<TYPE> p1, Point2Base<TYPE> p2);

//! Base class template for 2D sizes
template <class TYPE>
struct Size2Base
{
    union
    {
        struct
        {
            TYPE dx, dy;
        };
        TYPE data[2];
    };

    inline Size2Base();
    inline Size2Base(TYPE _dx, TYPE _dy);
    inline Size2Base(const Size2Base<TYPE>& Size);

    inline Size2Base<TYPE>& operator=(const Size2Base<TYPE>& Size);
    inline bool operator==(const Size2Base<TYPE>& _s) const;
    inline bool operator!=(const Size2Base<TYPE>& _s) const;
};

//! Base class template for 2D rects
template <class TYPE>
struct Rect2Base
{
    TYPE x;
    TYPE y;
    TYPE dx;
    TYPE dy;

    inline Rect2Base();
    inline Rect2Base(TYPE _x, TYPE _y, TYPE _dx, TYPE _dy);
    inline Rect2Base(const Rect2Base<TYPE>& Rect);
    inline Rect2Base(const Point2Base<TYPE>& Point, const Size2Base<TYPE>& Size);
    inline bool PointInside(const Point2Base<TYPE>& Point) const;
    inline bool RectInside(const Rect2Base<TYPE>& rect) const;
    inline Rect2Base<TYPE> Intersection(const Rect2Base<TYPE>& Rect) const;
    inline bool RectIntersects(const Rect2Base<TYPE>& Rect) const;

    inline Point2Base<TYPE> GetCenter() const;
    inline Point2Base<TYPE> GetPosition() const;
    inline Size2Base<TYPE> GetSize() const;

    inline void SetCenter(const Point2Base<TYPE>& center);
    inline void SetPosition(const Point2Base<TYPE>& position);
    inline void SetSize(const Size2Base<TYPE>& size);

    inline Rect2Base<TYPE>& operator=(const Rect2Base<TYPE>& rect);

    inline bool operator==(const Rect2Base<TYPE>& _s) const;
    inline bool operator!=(const Rect2Base<TYPE>& _s) const;

    inline Rect2Base<TYPE>& operator+=(const Point2Base<TYPE>& Point);
    inline Rect2Base<TYPE>& operator-=(const Point2Base<TYPE>& Point);

    inline Rect2Base<TYPE> operator+(const Point2Base<TYPE>& Point) const;
    inline Rect2Base<TYPE> operator-(const Point2Base<TYPE>& Point) const;
};

// Point2Base implementation

template <class TYPE>
inline Point2Base<TYPE>::Point2Base()
{
    x = y = 0;
}

template <class TYPE>
inline Point2Base<TYPE>::Point2Base(TYPE _x, TYPE _y)
{
    x = _x;
    y = _y;
};

template <class TYPE>
inline Point2Base<TYPE>::Point2Base(const Point2Base<TYPE>& Point)
{
    x = Point.x;
    y = Point.y;
}

template <class TYPE>
inline TYPE LineLength(Point2Base<TYPE> p1, Point2Base<TYPE> p2)
{
    return static_cast<TYPE>(sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y)));
}

// Size2Base implementation

template <class TYPE>
inline Size2Base<TYPE>::Size2Base()
{
    dx = dy = 0;
};

template <class TYPE>
inline Size2Base<TYPE>::Size2Base(TYPE _dx, TYPE _dy)
{
    dx = _dx;
    dy = _dy;
};

template <class TYPE>
inline Size2Base<TYPE>::Size2Base(const Size2Base<TYPE>& Size)
{
    dx = Size.dx;
    dy = Size.dy;
};

template <class TYPE>
inline Size2Base<TYPE>& Size2Base<TYPE>::operator=(const Size2Base<TYPE>& Size)
{
    dx = Size.dx;
    dy = Size.dy;
    return *this;
}

template <class TYPE>
inline Rect2Base<TYPE>::Rect2Base(const Point2Base<TYPE>& Point, const Size2Base<TYPE>& Size)
{
    x = Point.x;
    y = Point.y;
    dx = Size.dx;
    dy = Size.dy;
};

template <class TYPE>
inline Rect2Base<TYPE>& Rect2Base<TYPE>::operator=(const Rect2Base<TYPE>& rect)
{
    x = rect.x;
    y = rect.y;
    dx = rect.dx;
    dy = rect.dy;
    return *this;
}

template <class TYPE>
inline Point2Base<TYPE>& Point2Base<TYPE>::operator+=(const Point2Base<TYPE>& Point)
{
    x += Point.x;
    y += Point.y;
    return (*this);
};

template <class TYPE>
inline Point2Base<TYPE>& Point2Base<TYPE>::operator-=(const Point2Base<TYPE>& Point)
{
    x -= Point.x;
    y -= Point.y;
    return *this;
};

template <class TYPE>
inline Point2Base<TYPE> Point2Base<TYPE>::operator+(const Point2Base<TYPE>& Point) const
{
    return Point2Base<TYPE>(x + Point.x, y + Point.y);
}

template <class TYPE>
inline Point2Base<TYPE> Point2Base<TYPE>::operator-(const Point2Base<TYPE>& Point) const
{
    return Point2Base<TYPE>(x - Point.x, y - Point.y);
}

template <class TYPE>
inline Point2Base<TYPE> Point2Base<TYPE>::operator*(const Matrix2& Matrix) const
{
    return Point2Base<TYPE>(x * Matrix._00 + y * Matrix._10,
                            x * Matrix._01 + y * Matrix._11);
}

template <class TYPE>
inline Point2Base<TYPE>& Point2Base<TYPE>::operator*=(const Matrix2& Matrix)
{
    return (*this = *this * Matrix);
}

template <class TYPE>
inline Point2Base<TYPE> Point2Base<TYPE>::operator*(const TYPE t) const
{
    return Point2Base<TYPE>(x * t, y * t);
}

template <class TYPE>
inline Point2Base<TYPE>& Point2Base<TYPE>::operator*=(const TYPE t)
{
    return (*this = *this * t);
}

template <class TYPE>
inline Point2Base<TYPE> Point2Base<TYPE>::operator*(const Matrix3& Matrix) const
{
    return Point2Base<TYPE>(x * Matrix._00 + y * Matrix._10 + Matrix._20,
                            x * Matrix._01 + y * Matrix._11 + Matrix._21);
}

template <class TYPE>
inline Point2Base<TYPE>& Point2Base<TYPE>::operator*=(const Matrix3& Matrix)
{
    return (*this = *this * Matrix);
}

template <class TYPE>
inline bool Point2Base<TYPE>::operator==(const Point2Base<TYPE>& _p) const
{
    return (x == _p.x && y == _p.y);
}

template <class TYPE>
inline bool Point2Base<TYPE>::operator!=(const Point2Base<TYPE>& _p) const
{
    return (x != _p.x || y != _p.y);
}

template <class TYPE>
inline TYPE Point2Base<TYPE>::Length()
{
    return static_cast<TYPE>(sqrt(x * x + y * y));
}

template <class TYPE>
inline void Point2Base<TYPE>::Normalize()
{
    TYPE Len = Length();
    x /= Len;
    y /= Len;
}

// Size2Base implementation

template <class TYPE>
inline bool Size2Base<TYPE>::operator==(const Size2Base<TYPE>& _s) const
{
    return (dx == _s.dx && dy == _s.dy);
}

template <class TYPE>
inline bool Size2Base<TYPE>::operator!=(const Size2Base<TYPE>& _s) const
{
    return (dx != _s.dx || dy != _s.dy);
}

// Rect2Base implementation
template <class TYPE>
inline Rect2Base<TYPE>::Rect2Base()
{
    x = y = dx = dy = 0;
};

template <class TYPE>
inline Rect2Base<TYPE>::Rect2Base(TYPE _x, TYPE _y, TYPE _dx, TYPE _dy)
{
    x = _x;
    y = _y;
    dx = _dx;
    dy = _dy;
};

template <class TYPE>
inline Rect2Base<TYPE>::Rect2Base(const Rect2Base<TYPE>& Rect)
{
    x = Rect.x;
    y = Rect.y;
    dx = Rect.dx;
    dy = Rect.dy;
};

template <class TYPE>
inline bool Rect2Base<TYPE>::PointInside(const Point2Base<TYPE>& Point) const
{
    return ((Point.x >= x) && (Point.x <= x + dx) && (Point.y >= y) && (Point.y <= y + dy));
}

template <class TYPE>
inline bool Rect2Base<TYPE>::RectInside(const Rect2Base<TYPE>& rect) const
{
    return (PointInside({ rect.x, rect.y }) && PointInside({ rect.x + rect.dx, rect.y + rect.dy }));
}

template <class TYPE>
inline bool Rect2Base<TYPE>::RectIntersects(const Rect2Base<TYPE>& Rect) const
{
    int left1, left2;
    int right1, right2;
    int top1, top2;
    int bottom1, bottom2;

    left1 = x;
    left2 = Rect.x;
    right1 = x + dx;
    right2 = Rect.x + Rect.dx;
    top1 = y;
    top2 = Rect.y;
    bottom1 = y + dy;
    bottom2 = Rect.y + Rect.dy;

    if (bottom1 < top2)
        return (false);
    if (top1 > bottom2)
        return (false);

    if (right1 < left2)
        return (false);
    if (left1 > right2)
        return (false);

    return (true);
}

template <class TYPE>
inline Rect2Base<TYPE> Rect2Base<TYPE>::Intersection(const Rect2Base<TYPE>& Rect) const
{
    TYPE nx = Max(x, Rect.x);
    TYPE ny = Max(y, Rect.y);
    TYPE ndx = Min((dx + x) - nx, (Rect.dx + Rect.x) - nx);
    TYPE ndy = Min((dy + y) - ny, (Rect.dy + Rect.y) - ny);
    if (ndx <= 0 || ndy <= 0)
    {
        ndx = 0;
        ndy = 0;
    }

    return Rect2Base<TYPE>(nx, ny, ndx, ndy);
}

template <class TYPE>
inline Point2Base<TYPE> Rect2Base<TYPE>::GetCenter() const
{
    return Point2Base<TYPE>(x + (dx * .5f), y + (dy * .5f));
}

template <class TYPE>
inline Point2Base<TYPE> Rect2Base<TYPE>::GetPosition() const
{
    return Point2Base<TYPE>(x, y);
}

template <class TYPE>
inline Size2Base<TYPE> Rect2Base<TYPE>::GetSize() const
{
    return Size2Base<TYPE>(dx, dy);
}

template <class TYPE>
inline void Rect2Base<TYPE>::SetCenter(const Point2Base<TYPE>& center)
{
    x = static_cast<TYPE>(center.x - dx * 0.5f);
    y = static_cast<TYPE>(center.y - dy * 0.5f);
}

template <class TYPE>
inline void Rect2Base<TYPE>::SetPosition(const Point2Base<TYPE>& position)
{
    x = position.x;
    y = position.y;
}

template <class TYPE>
inline void Rect2Base<TYPE>::SetSize(const Size2Base<TYPE>& size)
{
    dx = size.dx;
    dy = size.dy;
}

template <class TYPE>
inline bool Rect2Base<TYPE>::operator==(const Rect2Base<TYPE>& _r) const
{
    return (x == _r.x && y == _r.y && dx == _r.dx && dy == _r.dy);
}

template <class TYPE>
inline bool Rect2Base<TYPE>::operator!=(const Rect2Base<TYPE>& _r) const
{
    return (!Rect2Base<TYPE>::operator==(_r));
}

template <class TYPE>
inline Rect2Base<TYPE>& Rect2Base<TYPE>::operator+=(const Point2Base<TYPE>& pt)
{
    x += pt.x;
    y += pt.y;
    return *this;
}

template <class TYPE>
inline Rect2Base<TYPE>& Rect2Base<TYPE>::operator-=(const Point2Base<TYPE>& pt)
{
    x -= pt.x;
    y -= pt.y;
    return *this;
}

template <class TYPE>
inline Rect2Base<TYPE> Rect2Base<TYPE>::operator+(const Point2Base<TYPE>& pt) const
{
    return Rect2Base<TYPE>(x + pt.x, y + pt.y, dx, dy);
}

template <class TYPE>
inline Rect2Base<TYPE> Rect2Base<TYPE>::operator-(const Point2Base<TYPE>& pt) const
{
    return Rect2Base<TYPE>(x - pt.x, y - pt.y, dx, dy);
}

}; // end of namespace DAVA
