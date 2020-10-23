#ifndef __DAVAENGINE_IRENDERUPDATABLE_H__
#define __DAVAENGINE_IRENDERUPDATABLE_H__

#include "Base/BaseObject.h"

namespace DAVA
{
//! Interface to retrieve updates
class Camera;
class IRenderUpdatable
{
public:
    virtual ~IRenderUpdatable() = default;
    virtual void RenderUpdate(Camera* camera, float32 timeElapsed) = 0;
};
};


#endif // __DAVAENGINE_IRENDERUPDATABLE_H__
