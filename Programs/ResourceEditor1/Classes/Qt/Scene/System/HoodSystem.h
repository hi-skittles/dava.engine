#pragma once

#include "Scene/SceneTypes.h"
#include "Classes/Selection/SelectableGroup.h"
#include "Scene/System/HoodSystem/NormalHood.h"
#include "Scene/System/HoodSystem/MoveHood.h"
#include "Scene/System/HoodSystem/ScaleHood.h"
#include "Scene/System/HoodSystem/RotateHood.h"
#include "SystemDelegates.h"

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

// bullet
#include "bullet/btBulletCollisionCommon.h"

// framework
#include "Entity/SceneSystem.h"
#include "UI/UIEvent.h"

class SceneCameraSystem;

class HoodSystem : public DAVA::SceneSystem, public SelectionSystemDelegate, public EditorSceneSystem
{
public:
    HoodSystem(DAVA::Scene* scene, SceneCameraSystem* camSys);
    ~HoodSystem();

    void SetTransformType(Selectable::TransformType mode);
    Selectable::TransformType GetTransformType() const;

    DAVA::Vector3 GetPosition() const;
    void SetPosition(const DAVA::Vector3& pos);

    void SetModifOffset(const DAVA::Vector3& offset);
    void SetModifRotate(const DAVA::float32& angle);
    void SetModifScale(const DAVA::float32& scale);

    void SetModifAxis(ST_Axis axis);
    ST_Axis GetModifAxis() const;
    ST_Axis GetPassingAxis() const;

    void SetScale(DAVA::float32 scale);
    DAVA::float32 GetScale() const;

    void LockScale(bool lock);
    void LockModif(bool lock);
    void LockAxis(bool lock);

    void SetVisible(bool visible);
    bool IsVisible() const;

    void Process(DAVA::float32 timeElapsed) override;
    void PrepareForRemove() override
    {
    }
    bool Input(DAVA::UIEvent* event) override;

protected:
    void Draw() override;

private:
    void AddCollObjects(const DAVA::Vector<HoodCollObject*>* objects);
    void RemCollObjects(const DAVA::Vector<HoodCollObject*>* objects);
    void ResetModifValues();

    bool AllowPerformSelectionHavingCurrent(const SelectableGroup& currentSelection) override;
    bool AllowChangeSelectionReplacingCurrent(const SelectableGroup& currentSelection, const SelectableGroup& newSelection) override;

    btCollisionWorld* collWorld = nullptr;
    btAxisSweep3* collBroadphase = nullptr;
    btDefaultCollisionConfiguration* collConfiguration = nullptr;
    btCollisionDispatcher* collDispatcher = nullptr;
    btIDebugDraw* collDebugDraw = nullptr;
    HoodObject* curHood = nullptr;
    SceneCameraSystem* cameraSystem = nullptr;
    NormalHood normalHood;
    MoveHood moveHood;
    RotateHood rotateHood;
    ScaleHood scaleHood;
    DAVA::Vector3 curPos;
    DAVA::float32 curScale = 1.0f;
    DAVA::Vector3 modifOffset;
    Selectable::TransformType curMode = Selectable::TransformType::Disabled;
    ST_Axis curAxis = ST_AXIS_NONE;
    ST_Axis moseOverAxis = ST_AXIS_NONE;
    bool lockedScale = false;
    bool lockedModif = false;
    bool lockedAxis = false;
    bool isVisible = true;
};
