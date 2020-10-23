#include "WaypointsInterpolator.h"

float32 WaypointsInterpolator::SPLINE_DELTA_TIME = 0.00001f;

WaypointsInterpolator::WaypointsInterpolator(const Vector<PathComponent::Waypoint*>& _waypoints, float32 _time)
    : waypoints(_waypoints)
    , segment(0)
    , segmentTime(0.0f)
    , targetSegmentTime(0.0f)
    , splineTime(_time)
    , splineLength(0.0f)
{
    Init();
}

void WaypointsInterpolator::Init()
{
    DVASSERT(waypoints.size() > 3);

    spline = std::unique_ptr<BasicSpline3>(new BasicSpline3());
    Polygon3 poly;

    for (auto* point : waypoints)
    {
        poly.AddPoint(point->position);
    }

    spline->Construct(poly);

    for (int32 i = 0; i < spline->pointCount - 1; i++)
    {
        float32 t = 0.0f;
        float32 length = 0.0f;

        Vector3 prev = spline->Evaluate(i, t);

        while (t <= 1.0f)
        {
            const Vector3& current = spline->Evaluate(i, t);
            length += (current - prev).Length();

            prev = current;
            t += SPLINE_DELTA_TIME;
        }

        splineLength += length;
        segmentsLength.push_back(length);
    }

    for (uint32 i = 0; i < segmentsLength.size(); i++)
    {
        segmentsLength[i] = segmentsLength[i] / splineLength * splineTime;
    }

    currentPosition = waypoints[0]->position;
    targetPosition = waypoints[1]->position;

    targetSegmentTime = segmentsLength[0];
}

void WaypointsInterpolator::NextPosition(Vector3& position, Vector3& target, float32 timeElapsed)
{
    position = currentPosition;
    target = targetPosition;

    segmentTime += timeElapsed;

    if (segmentTime >= targetSegmentTime)
    {
        segment = (segment == spline->pointCount - 2) ? 0 : segment + 1;
        segmentTime -= targetSegmentTime;
        targetSegmentTime = segmentsLength[segment];
    }

    Vector3 tangent = spline->EvaluateDerivative(segment, segmentTime / targetSegmentTime);
    tangent.Normalize();

    currentPosition = spline->Evaluate(segment, segmentTime / targetSegmentTime);
    targetPosition = currentPosition + tangent;
}