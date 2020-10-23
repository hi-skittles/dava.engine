#include "Core/PerformanceSettings.h"
namespace DAVA
{
PerformanceSettings::PerformanceSettings()
{
    //init with some default values
    psPerformanceMinFPS = 35.f;
    psPerformanceMaxFPS = 50.f;
    psPerformanceLodOffset = 11.7f;
    psPerformanceLodMult = 3.9f;
    psPerformanceSpeedMult = 2.f;
}
float32 PerformanceSettings::GetPsPerformanceMinFPS()
{
    return psPerformanceMinFPS;
}
float32 PerformanceSettings::GetPsPerformanceMaxFPS()
{
    return psPerformanceMaxFPS;
}
float32 PerformanceSettings::GetPsPerformanceSpeedMult()
{
    return psPerformanceSpeedMult;
}
float32 PerformanceSettings::GetPsPerformanceLodOffset()
{
    return psPerformanceLodOffset;
}
float32 PerformanceSettings::GetPsPerformanceLodMult()
{
    return psPerformanceLodMult;
}

void PerformanceSettings::SetPsPerformanceMinFPS(float32 minFPS)
{
    psPerformanceMinFPS = minFPS;
}
void PerformanceSettings::SetPsPerformanceMaxFPS(float32 maxFPS)
{
    psPerformanceMaxFPS = maxFPS;
}
void PerformanceSettings::SetPsPerformanceSpeedMult(float32 speedMult)
{
    psPerformanceSpeedMult = speedMult;
}
void PerformanceSettings::SetPsPerformanceLodOffset(float32 lodOffset)
{
    psPerformanceLodOffset = lodOffset;
}
void PerformanceSettings::SetPsPerformanceLodMult(float32 lodMult)
{
    psPerformanceLodMult = lodMult;
}
}