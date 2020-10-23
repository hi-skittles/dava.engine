#ifndef __DAVAENGINE_CIRCLE_H__
#define __DAVAENGINE_CIRCLE_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Circle
 */
class Circle
{
public:
    float32 radius;
    float32 squareRadius;
    Vector2 center;
};
};

#endif // __DAVAENGINE_CIRCLE_H__