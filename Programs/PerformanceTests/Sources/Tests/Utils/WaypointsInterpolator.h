#ifndef __CAMERA_CONTROLLER_H__
#define __CAMERA_CONTROLLER_H__

#include "DAVAEngine.h"

using namespace DAVA;

class WaypointsInterpolator
{
public:
    WaypointsInterpolator(const Vector<PathComponent::Waypoint*>& waypoints, float32 time);

    void NextPosition(Vector3& position, Vector3& target, float32 timeElapsed);

    void SetWaypoints(const Vector<PathComponent::Waypoint*>& waypoints);

    void SetStep(float32 step);
    float32 GetStep() const;

private:
    void Init();

    static float32 SPLINE_DELTA_TIME;

    Vector<PathComponent::Waypoint*> waypoints;
    Vector<float32> segmentsLength;

    Vector3 currentPosition;
    Vector3 targetPosition;

    uint32 segment;
    float32 segmentTime;
    float32 targetSegmentTime;
    float32 splineTime;

    float32 splineLength;

    std::unique_ptr<BasicSpline3> spline;
};

inline void WaypointsInterpolator::SetWaypoints(const Vector<PathComponent::Waypoint*>& _waypoints)
{
    waypoints = _waypoints;

    Init();
}

#endif