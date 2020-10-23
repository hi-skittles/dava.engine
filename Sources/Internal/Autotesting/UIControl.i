%module UIControl
%{
#include "UI/UIControl.h"
%}

%import Vector.i
%import Rect.i
%import Polygon2.i

namespace DAVA
{
	
class UIGeometricData
{
	
public:
	UIGeometricData()
	{
		cosA = 1.0f;
		sinA = 0;
		oldAngle = 0;
		angle = 0;
	}
	Vector2 position;
	Vector2 size;
	
	Vector2 pivotPoint;
	Vector2	scale;
	float32	angle;
	
	float32 cosA;
	float32 sinA;

    void GetPolygon( Polygon2 &polygon ) const
    {
        polygon.Clear();
        polygon.points.reserve( 4 );
        polygon.AddPoint( Vector2() );
        polygon.AddPoint( Vector2( size.x, 0 ) );
        polygon.AddPoint( size );
        polygon.AddPoint( Vector2( 0, size.y ) );

        Matrix3 transformMtx;
        BuildTransformMatrix( transformMtx );
        polygon.Transform( transformMtx );
    }

	const Rect &GetUnrotatedRect() const
	{
		return unrotatedRect;
	}
};

class UIControl
{
public:

	UIControl(const Rect &rect = Rect());
	
	inline const Rect GetRect() const;
    
	inline const Vector2 &GetPosition() const;
	
	virtual const Vector2 &GetSize() const;

	virtual const UIGeometricData &GetGeometricData();

	inline bool GetVisibilityFlag() const;

	virtual bool GetInputEnabled() const;

	virtual void SetInputEnabled(bool isEnabled, bool hierarchic = true);
	
	virtual bool GetDisabled() const;

	virtual bool GetSelected() const;

	bool IsVisible() const;

	int32 GetTag() const;
    
	UIControl *GetParent();

	DAVA::int32 GetState() const;

	int32 GetFrame() const;

	Vector2 UIControl::GetPivotPoint() const;

protected:
	virtual ~UIControl();

};
};
