#include "SnapToLandscapeControllerSystem.h"

#include "Scene3D/Components/Controller/SnapToLandscapeControllerComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"

#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/EventSystem.h"

#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Landscape.h"

namespace DAVA
{
SnapToLandscapeControllerSystem::SnapToLandscapeControllerSystem(Scene* scene)
    : SceneSystem(scene)
{
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::SNAP_TO_LANDSCAPE_HEIGHT_CHANGED);
}

SnapToLandscapeControllerSystem::~SnapToLandscapeControllerSystem()
{
}

void SnapToLandscapeControllerSystem::AddEntity(Entity* entity)
{
    DVASSERT(GetCamera(entity) != NULL && "Right now system works with camera only");

    positions[entity] = Vector3();

    entities.push_back(entity);
}

void SnapToLandscapeControllerSystem::RemoveEntity(Entity* entity)
{
    uint32 size = static_cast<uint32>(entities.size());
    for (uint32 i = 0; i < size; ++i)
    {
        if (entities[i] == entity)
        {
            Map<Entity*, Vector3>::iterator it = positions.find(entity);
            if (it != positions.end())
            {
                positions.erase(it);
            }
            else
            {
                DVASSERT(false);
            }

            entities[i] = entities[size - 1];
            entities.pop_back();
            return;
        }
    }
    DVASSERT(0);
}

void SnapToLandscapeControllerSystem::PrepareForRemove()
{
    entities.clear();
    positions.clear();
}

void SnapToLandscapeControllerSystem::Process(float32 timeElapsed)
{
    const uint32 size = static_cast<uint32>(entities.size());
    if (0 == size)
        return;

    Landscape* landscape = FindLandscape(GetScene());
    for (uint32 i = 0; i < size; ++i)
    {
        SnapToLandscape(landscape, entities[i]);
    }
}

void SnapToLandscapeControllerSystem::ImmediateEvent(Component* component, uint32 event)
{
    if (EventSystem::SNAP_TO_LANDSCAPE_HEIGHT_CHANGED == event)
    {
        Landscape* landscape = FindLandscape(GetScene());
        SnapToLandscape(landscape, component->GetEntity(), true);
    }
}

void SnapToLandscapeControllerSystem::SnapToLandscape(Landscape* landscape, Entity* entity, bool forceSnap)
{
    if (!landscape)
        return;

    SnapToLandscapeControllerComponent* snapController = GetSnapToLandscapeControllerComponent(entity);
    Camera* camera = GetCamera(entity);
    DVASSERT(snapController && camera);

    if (camera && snapController)
    {
        const Vector3& pos = camera->GetPosition();
        const Vector3& prevPos = positions[entity];
        if ((pos != prevPos) || forceSnap)
        {
            const Vector3& direction = camera->GetDirection();

            Vector3 placedPos = pos;
            Vector3 pointOnLandscape = pos;
            if (pos != prevPos) //need check landscape edges only in case of position changing
            {
                const AABBox3& landBox = landscape->GetBoundingBox();

                if ((pos.x > landBox.max.x || pos.x < landBox.min.x)
                    || (pos.y > landBox.max.y || pos.y < landBox.min.y))
                {
                    placedPos.x = Clamp(pos.x, landBox.min.x, landBox.max.x);
                    placedPos.y = Clamp(pos.y, landBox.min.y, landBox.max.y);
                }
            }

            bool placed = landscape->PlacePoint(placedPos, pointOnLandscape);
            if (placed)
            {
                pointOnLandscape.z += snapController->GetHeightOnLandscape();
            }

            camera->SetPosition(pointOnLandscape);
            camera->SetDirection(direction);

            positions[entity] = pointOnLandscape;
        }
    }
}
};
