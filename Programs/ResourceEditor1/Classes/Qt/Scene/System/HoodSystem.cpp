#include "Classes/SceneManager/SceneData.h"
#include "Classes/Application/REGlobal.h"

#include "Classes/Qt/Scene/System/HoodSystem.h"
#include "Classes/Qt/Scene/System/ModifSystem.h"
#include "Classes/Qt/Scene/System/CollisionSystem.h"
#include "Classes/Qt/Scene/System/CameraSystem.h"
#include "Classes/Qt/Scene/SceneEditor2.h"

#include <Scene3D/Scene.h>
#include <Base/AlignedAllocator.h>

class SceneCollisionDebugDrawer final : public btIDebugDraw
{
public:
    SceneCollisionDebugDrawer(DAVA::RenderHelper* drawer_)
        : dbgMode(0)
        , drawer(drawer_)
    {
    }

    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
    {
        DAVA::Vector3 davaFrom(from.x(), from.y(), from.z());
        DAVA::Vector3 davaTo(to.x(), to.y(), to.z());
        DAVA::Color davaColor(color.x(), color.y(), color.z(), 1.0f);

        drawer->DrawLine(davaFrom, davaTo, davaColor, DAVA::RenderHelper::DRAW_WIRE_DEPTH);
    }

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override
    {
        DAVA::Color davaColor(color.x(), color.y(), color.z(), 1.0f);
        drawer->DrawIcosahedron(DAVA::Vector3(PointOnB.x(), PointOnB.y(), PointOnB.z()), distance / 20.f, davaColor, DAVA::RenderHelper::DRAW_SOLID_DEPTH);
    }

    void reportErrorWarning(const char* warningString) override
    {
    }
    void draw3dText(const btVector3& location, const char* textString) override
    {
    }
    void setDebugMode(int debugMode) override
    {
        dbgMode = debugMode;
    }
    int getDebugMode() const override
    {
        return dbgMode;
    }

protected:
    int dbgMode;
    DAVA::RenderHelper* drawer;
};

HoodSystem::HoodSystem(DAVA::Scene* scene, SceneCameraSystem* camSys)
    : DAVA::SceneSystem(scene)
    , cameraSystem(camSys)
{
    btVector3 worldMin(-1000, -1000, -1000);
    btVector3 worldMax(1000, 1000, 1000);

    collConfiguration = new btDefaultCollisionConfiguration();
    collDispatcher = DAVA::CreateObjectAligned<btCollisionDispatcher, 16>(collConfiguration);
    collBroadphase = new btAxisSweep3(worldMin, worldMax);
    collDebugDraw = new SceneCollisionDebugDrawer(scene->GetRenderSystem()->GetDebugDrawer());
    collDebugDraw->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    collWorld = new btCollisionWorld(collDispatcher, collBroadphase, collConfiguration);
    collWorld->setDebugDrawer(collDebugDraw);

    SetModifAxis(ST_AXIS_X);
    SetTransformType(Selectable::TransformType::Translation);

    moveHood.colorX = DAVA::Color(1, 0, 0, 1);
    moveHood.colorY = DAVA::Color(0, 1, 0, 1);
    moveHood.colorZ = DAVA::Color(0, 0, 1, 1);
    moveHood.colorS = DAVA::Color(1, 1, 0, 1);

    rotateHood.colorX = DAVA::Color(1, 0, 0, 1);
    rotateHood.colorY = DAVA::Color(0, 1, 0, 1);
    rotateHood.colorZ = DAVA::Color(0, 0, 1, 1);
    rotateHood.colorS = DAVA::Color(1, 1, 0, 1);

    scaleHood.colorX = DAVA::Color(1, 0, 0, 1);
    scaleHood.colorY = DAVA::Color(0, 1, 0, 1);
    scaleHood.colorZ = DAVA::Color(0, 0, 1, 1);
    scaleHood.colorS = DAVA::Color(1, 1, 0, 1);

    normalHood.colorX = DAVA::Color(0.7f, 0.3f, 0.3f, 1);
    normalHood.colorY = DAVA::Color(0.3f, 0.7f, 0.3f, 1);
    normalHood.colorZ = DAVA::Color(0.3f, 0.3f, 0.7f, 1);
    normalHood.colorS = DAVA::Color(0, 0, 0, 1);
}

HoodSystem::~HoodSystem()
{
    delete collWorld;
    delete collDebugDraw;
    delete collBroadphase;
    DAVA::DestroyObjectAligned(collDispatcher);
    delete collConfiguration;
}

DAVA::Vector3 HoodSystem::GetPosition() const
{
    return (curPos + modifOffset);
}

void HoodSystem::SetPosition(const DAVA::Vector3& pos)
{
    if (!IsLocked() && !lockedScale)
    {
        if (curPos != pos || !modifOffset.IsZero())
        {
            curPos = pos;
            ResetModifValues();

            if (NULL != curHood)
            {
                curHood->UpdatePos(curPos);
                normalHood.UpdatePos(curPos);

                collWorld->updateAabbs();
            }
        }
    }
}

void HoodSystem::SetModifOffset(const DAVA::Vector3& offset)
{
    if (!IsLocked())
    {
        moveHood.modifOffset = offset;

        if (modifOffset != offset)
        {
            modifOffset = offset;

            if (NULL != curHood)
            {
                curHood->UpdatePos(curPos + modifOffset);
                normalHood.UpdatePos(curPos + modifOffset);

                collWorld->updateAabbs();
            }
        }
    }
}

void HoodSystem::SetModifRotate(const DAVA::float32& angle)
{
    if (!IsLocked())
    {
        rotateHood.modifRotate = angle;
    }
}

void HoodSystem::SetModifScale(const DAVA::float32& scale)
{
    if (!IsLocked())
    {
        scaleHood.modifScale = scale;
    }
}

void HoodSystem::SetScale(DAVA::float32 scale)
{
    if (!IsLocked())
    {
        scale = scale * REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>()->gizmoScale;

        if (curScale != scale && 0 != scale)
        {
            curScale = scale;

            if (NULL != curHood)
            {
                curHood->UpdateScale(curScale);
                normalHood.UpdateScale(curScale);

                collWorld->updateAabbs();
            }
        }
    }
}

DAVA::float32 HoodSystem::GetScale() const
{
    return curScale;
}

void HoodSystem::SetTransformType(Selectable::TransformType mode)
{
    if (!IsLocked())
    {
        if (curMode != mode)
        {
            if (NULL != curHood)
            {
                RemCollObjects(&curHood->collObjects);
            }

            curMode = mode;
            switch (mode)
            {
            case Selectable::TransformType::Translation:
                curHood = &moveHood;
                break;
            case Selectable::TransformType::Scale:
                curHood = &scaleHood;
                break;
            case Selectable::TransformType::Rotation:
                curHood = &rotateHood;
                break;
            default:
                curHood = &normalHood;
                break;
            }

            if (NULL != curHood)
            {
                AddCollObjects(&curHood->collObjects);

                curHood->UpdatePos(curPos + modifOffset);
                curHood->UpdateScale(curScale);
            }

            collWorld->updateAabbs();
        }
    }
}

Selectable::TransformType HoodSystem::GetTransformType() const
{
    if (lockedModif)
    {
        return Selectable::TransformType::Disabled;
    }

    return curMode;
}

void HoodSystem::SetVisible(bool visible)
{
    if (!IsLocked())
    {
        isVisible = visible;
    }
}

bool HoodSystem::IsVisible() const
{
    return isVisible;
}

void HoodSystem::AddCollObjects(const DAVA::Vector<HoodCollObject*>* objects)
{
    if (NULL != objects)
    {
        for (size_t i = 0; i < objects->size(); ++i)
        {
            collWorld->addCollisionObject(objects->operator[](i)->btObject);
        }
    }
}

void HoodSystem::RemCollObjects(const DAVA::Vector<HoodCollObject*>* objects)
{
    if (NULL != objects)
    {
        for (size_t i = 0; i < objects->size(); ++i)
        {
            collWorld->removeCollisionObject(objects->operator[](i)->btObject);
        }
    }
}

void HoodSystem::ResetModifValues()
{
    modifOffset = DAVA::Vector3(0, 0, 0);

    rotateHood.modifRotate = 0;
    scaleHood.modifScale = 0;
}

void HoodSystem::Process(DAVA::float32 timeElapsed)
{
    InputLockGuard guard(GetScene(), this);
    if (guard.IsLockAcquired() == false)
    {
        return;
    }

    if (!IsLocked() && !lockedScale)
    {
        // scale hood depending on current camera position
        DAVA::Camera* curCamera = cameraSystem->GetCurCamera();
        if (NULL != curCamera)
        {
            DAVA::float32 camToHoodDist = (GetPosition() - curCamera->GetPosition()).Length();
            if (curCamera->GetIsOrtho())
            {
                SetScale(30.0f);
            }
            else
            {
                SetScale(camToHoodDist / 20.f);
            }
        }
    }
}

bool HoodSystem::Input(DAVA::UIEvent* event)
{
    InputLockGuard guard(GetScene(), this);
    if (guard.IsLockAcquired() == false)
    {
        return false;
    }

    if (!event->point.IsZero())
    {
        // before checking result mark that there is no hood axis under mouse
        if (!lockedScale && !lockedAxis)
        {
            moseOverAxis = ST_AXIS_NONE;

            // if is visible and not locked check mouse over status
            if (!lockedModif && NULL != curHood)
            {
                // get intersected items in the line from camera to current mouse position
                DAVA::Vector3 traceFrom;
                DAVA::Vector3 traceTo;

                cameraSystem->GetRayTo2dPoint(event->point, 99999.0f, traceFrom, traceTo);

                btVector3 btFrom(traceFrom.x, traceFrom.y, traceFrom.z);
                btVector3 btTo(traceTo.x, traceTo.y, traceTo.z);

                btCollisionWorld::AllHitsRayResultCallback btCallback(btFrom, btTo);
                collWorld->rayTest(btFrom, btTo, btCallback);

                if (btCallback.hasHit())
                {
                    const DAVA::Vector<HoodCollObject*>* curHoodObjects = &curHood->collObjects;
                    for (size_t i = 0; i < curHoodObjects->size(); ++i)
                    {
                        HoodCollObject* hObj = curHoodObjects->operator[](i);

                        if (hObj->btObject == btCallback.m_collisionObjects[0])
                        {
                            // mark that mouse is over one of hood axis
                            moseOverAxis = hObj->axis;
                            break;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void HoodSystem::Draw()
{
    InputLockGuard guard(GetScene(), this);
    if (guard.IsLockAcquired() == false)
    {
        return;
    }

    if ((curHood == nullptr) || !IsVisible())
        return;

    TextDrawSystem* textDrawSys = ((SceneEditor2*)GetScene())->textDrawSystem;

    // modification isn't locked and whole system isn't locked
    if (!IsLocked() && !lockedModif)
    {
        ST_Axis showAsSelected = curAxis;
        if ((GetTransformType() != Selectable::TransformType::Disabled) && (ST_AXIS_NONE != moseOverAxis))
        {
            showAsSelected = moseOverAxis;
        }

        curHood->Draw(showAsSelected, moseOverAxis, GetScene()->GetRenderSystem()->GetDebugDrawer(), textDrawSys);
        GetScene()->GetRenderSystem()->GetDebugDrawer()->DrawAABox(DAVA::AABBox3(GetPosition(), curHood->objScale * .04f), DAVA::Color::White, DAVA::RenderHelper::DRAW_SOLID_NO_DEPTH);
    }
    else
    {
        normalHood.Draw(curAxis, ST_AXIS_NONE, GetScene()->GetRenderSystem()->GetDebugDrawer(), textDrawSys);
    }
}

void HoodSystem::SetModifAxis(ST_Axis axis)
{
    if (ST_AXIS_NONE != axis)
    {
        curAxis = axis;
    }
}

ST_Axis HoodSystem::GetModifAxis() const
{
    return curAxis;
}

ST_Axis HoodSystem::GetPassingAxis() const
{
    return moseOverAxis;
}

void HoodSystem::LockScale(bool lock)
{
    lockedScale = lock;
}

void HoodSystem::LockModif(bool lock)
{
    lockedModif = lock;
}

void HoodSystem::LockAxis(bool lock)
{
    lockedAxis = lock;
}

bool HoodSystem::AllowPerformSelectionHavingCurrent(const SelectableGroup& currentSelection)
{
    return !IsVisible() || (GetTransformType() == Selectable::TransformType::Disabled) || (ST_AXIS_NONE == GetPassingAxis());
}

bool HoodSystem::AllowChangeSelectionReplacingCurrent(const SelectableGroup& currentSelection, const SelectableGroup& newSelection)
{
    return true;
}
