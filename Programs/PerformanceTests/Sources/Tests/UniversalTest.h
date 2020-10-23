#ifndef __UNIVERSAL_TEST_H__
#define __UNIVERSAL_TEST_H__

#include "Tests/BaseTest.h"
#include "Tests/Utils/WaypointsInterpolator.h"
#include "Tests/Utils/TankUtils.h"

class UniversalTest : public BaseTest
{
public:
    UniversalTest(const TestParams& params);

    static const String TEST_NAME;

protected:
    void LoadResources() override;
    void PerformTestLogic(float32 timeElapsed) override;

private:
    static const FastName CAMERA;

    static const FastName CAMERA_PATH;
    static const FastName TANK_STUB;
    static const FastName TANKS;

    static const float32 TANK_ROTATION_ANGLE;

    Map<FastName, std::pair<Entity*, Vector<uint32>>> skinnedTankData;
    List<Entity*> tankStubs;

    std::unique_ptr<WaypointsInterpolator> waypointInterpolator;

    ScopedPtr<Camera> camera;
    Vector3 camPos;
    Vector3 camDst;

    float32 time;
};

#endif
