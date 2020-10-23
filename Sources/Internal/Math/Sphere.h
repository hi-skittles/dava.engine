#ifndef __DAVAENGINE_SPHERE_H__
#define __DAVAENGINE_SPHERE_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"

namespace DAVA
{
/**	
	\ingroup math
	\brief Sphere
 */
class Sphere
{
public:
    float32 radius;
    float32 squareRadius;
    Vector3 center;
};
};

#endif // __DAVAENGINE_SPHERE_H__