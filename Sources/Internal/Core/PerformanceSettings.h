#ifndef __DAVAENGINE_PERFORMANCE_SETTINGS_H__
#define __DAVAENGINE_PERFORMANCE_SETTINGS_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/Singleton.h"

namespace DAVA
{
class PerformanceSettings : public Singleton<PerformanceSettings>
{
public:
    PerformanceSettings();
    float32 GetPsPerformanceMinFPS();
    float32 GetPsPerformanceMaxFPS();
    float32 GetPsPerformanceSpeedMult();
    float32 GetPsPerformanceLodOffset();
    float32 GetPsPerformanceLodMult();

    void SetPsPerformanceMinFPS(float32 minFPS);
    void SetPsPerformanceMaxFPS(float32 maxFPS);
    void SetPsPerformanceSpeedMult(float32 speedMult);
    void SetPsPerformanceLodOffset(float32 lodOffset);
    void SetPsPerformanceLodMult(float32 lodMult);

protected:
    float32 psPerformanceMinFPS;
    float32 psPerformanceMaxFPS;
    float32 psPerformanceSpeedMult;
    float32 psPerformanceLodOffset;
    float32 psPerformanceLodMult;
};
}

#endif