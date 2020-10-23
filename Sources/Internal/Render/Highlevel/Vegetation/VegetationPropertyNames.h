#ifndef __DAVAENGINE_VEGETATIONPROPERTYNAMES_H__
#define __DAVAENGINE_VEGETATIONPROPERTYNAMES_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "Render/RenderBase.h"
#include "Base/BaseMath.h"

namespace DAVA
{
/**
 \brief Different uniform and shader flag names
 */
class VegetationPropertyNames
{
public:
    static const FastName UNIFORM_TILEPOS;
    static const FastName UNIFORM_WORLD_SIZE;
    static const FastName UNIFORM_SWITCH_LOD_SCALE;
    static const FastName UNIFORM_PERTURBATION_FORCE;
    static const FastName UNIFORM_PERTURBATION_POINT;
    static const FastName UNIFORM_PERTURBATION_FORCE_DISTANCE;

    static const FastName FLAG_LOD_COLOR;

    static const FastName VEGETATION_QUALITY_NAME_HIGH;
    static const FastName VEGETATION_QUALITY_NAME_LOW;
    static const FastName VEGETATION_QUALITY_GROUP_NAME;

    static const FastName UNIFORM_SAMPLER_VEGETATIONMAP;

    static const FastName UNIFORM_VEGWAVEOFFSET_X;
    static const FastName UNIFORM_VEGWAVEOFFSET_Y;
};
};

#endif /* defined(__DAVAENGINE_VEGETATIONPROPERTYNAMES_H__) */
