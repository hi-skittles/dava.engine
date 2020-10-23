#include "UniversalTest.h"

const FastName UniversalTest::CAMERA_PATH = FastName("CameraPath");
const FastName UniversalTest::TANK_STUB = FastName("TankStub");
const FastName UniversalTest::TANKS = FastName("Tanks");
const FastName UniversalTest::CAMERA = FastName("Camera");

const String UniversalTest::TEST_NAME = "UniversalTest";

const float32 UniversalTest::TANK_ROTATION_ANGLE = 45.0f;

UniversalTest::UniversalTest(const TestParams& params)
    : BaseTest(TEST_NAME, params)
    , camera(new Camera())
    , time(0.0f)
{
}

void UniversalTest::LoadResources()
{
    BaseTest::LoadResources();

    SceneFileV2::eError error = GetScene()->LoadScene(FilePath("~res:/3d/Maps/" + GetParams().scenePath));
    DVASSERT(error == SceneFileV2::eError::ERROR_NO_ERROR, ("can't load scene " + GetParams().scenePath).c_str());

    Entity* cameraEntity = GetScene()->FindByName(CAMERA);

    if (cameraEntity != nullptr)
    {
        Camera* camera = cameraEntity->GetComponent<CameraComponent>()->GetCamera();
        GetScene()->SetCurrentCamera(camera);
    }
    else
    {
        Entity* cameraPathEntity = GetScene()->FindByName(CAMERA_PATH);
        DVASSERT(cameraPathEntity != nullptr, "Can't get path component");

        PathComponent* pathComponent = cameraPathEntity->GetComponent<PathComponent>();

        const Vector3& startPosition = pathComponent->GetStartWaypoint()->position;
        const Vector3& destinationPoint = pathComponent->GetStartWaypoint()->edges[0]->destination->position;

        camera->SetPosition(startPosition);
        camera->SetTarget(destinationPoint);
        camera->SetUp(Vector3::UnitZ);
        camera->SetLeft(Vector3::UnitY);

        GetScene()->SetCurrentCamera(camera);

        waypointInterpolator = std::unique_ptr<WaypointsInterpolator>(new WaypointsInterpolator(pathComponent->GetPoints(), GetParams().targetTime / 1000.0f));
    }

    Entity* tanksEntity = GetScene()->FindByName(TANKS);

    if (tanksEntity != nullptr)
    {
        Vector<Entity*> tanks;

        uint32 childrenCount = tanksEntity->GetChildrenCount();

        for (uint32 i = 0; i < childrenCount; i++)
        {
            tanks.push_back(tanksEntity->GetChild(i));
        }

        for (auto* tank : tanks)
        {
            Vector<uint32> jointsInfo;

            TankUtils::MakeSkinnedTank(tank, jointsInfo);
            skinnedTankData.insert(std::make_pair(tank->GetName(), std::make_pair(tank, jointsInfo)));
        }

        GetScene()->FindNodesByNamePart(TANK_STUB.c_str(), tankStubs);

        auto tankIt = skinnedTankData.cbegin();
        auto tankEnd = skinnedTankData.cend();

        for (auto* tankStub : tankStubs)
        {
            Entity* tank = tankIt->second.first;

            tankStub->AddNode(ScopedPtr<Entity>(tank->Clone()));

            tankIt++;
            if (tankIt == tankEnd)
            {
                tankIt = skinnedTankData.cbegin();
            }
        }
    }
}

void UniversalTest::PerformTestLogic(float32 timeElapsed)
{
    time += timeElapsed;

    if (waypointInterpolator)
    {
        waypointInterpolator->NextPosition(camPos, camDst, timeElapsed);

        camera->SetPosition(camPos);
        camera->SetTarget(camDst);
    }

    for (auto* tank : tankStubs)
    {
        const Vector<uint32>& jointIndexes = skinnedTankData.at(tank->GetChild(0)->GetName()).second;
        TankUtils::Animate(tank, jointIndexes, DegToRad(time * TANK_ROTATION_ANGLE));
    }
}
