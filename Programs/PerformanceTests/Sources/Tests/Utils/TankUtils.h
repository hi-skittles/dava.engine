#ifndef __CONQUEROR_ANIMATOR_H__
#define __CONQUEROR_ANIMATOR_H__

#include "DAVAEngine.h"
#include "Infrastructure/Settings/GraphicsDetect.h"

namespace DAVA
{
namespace TankUtils
{
struct TankNode
{
    const static FastName TURRET;
    const static FastName L_WHEELS;
    const static FastName R_WHEELS;
    const static FastName GUN_SHOT;
    const static FastName SKINNED_TANK;
};

void Animate(Entity* skinnedTank, const Vector<uint32>& jointIndexes, float32 angle);
void MakeSkinnedTank(Entity* entity, Vector<uint32>& jointsInfo);
}
}

#endif