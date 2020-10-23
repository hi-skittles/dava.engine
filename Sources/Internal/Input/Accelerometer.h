#ifndef __DAVAENGINE_ACCELEROMETER_H__
#define __DAVAENGINE_ACCELEROMETER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/Singleton.h"
#include "Base/EventDispatcher.h"

namespace DAVA
{
#define DEFAULT_UPDATE_RATE 1.0f / 10.0f

/**
	\ingroup input
	\brief Accelerometer access. Available only on devices that have accelerometer. 
 */
class Accelerometer : public Singleton<Accelerometer>
{
    IMPLEMENT_EVENT_DISPATCHER(eventDispatcher)

public:
    enum
    {
        EVENT_ACCELLEROMETER_DATA = 1,
    };

    Accelerometer();
    virtual ~Accelerometer();

    /*
	 Always return true for iPhone, but not supported by PSP & Nintendo DS
	 By default if nothing provided by platform layer this function return false
	 */
    virtual bool IsSupportedByHW()
    {
        return false;
    }
    /*
	 
	*/
    virtual const Vector3& GetAccelerationData()
    {
        return accelerationData;
    }

    virtual void Enable(float32 updateRate = DEFAULT_UPDATE_RATE);
    virtual void Disable()
    {
    }
    virtual bool IsEnabled() const
    {
        return false;
    }

protected:
    Vector3 accelerationData;
};
};
#endif // __DAVAENGINE_ACCELEROMETER_H__
