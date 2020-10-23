#pragma once

#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtConnections.h>

namespace DAVA
{
class FieldBinder;
}

struct ModificationData;
struct ModificationInternalData;

class ModificationModule : public DAVA::ClientModule
{
private:
    void PostInit() override;
    void OnContextCreated(DAVA::DataContext* context) override;

    void BindData();
    void CreateToolbar();
    void CreateActions();
    QWidget* CreateManualModificationControlBar();

    DAVA::SceneEditor2* GetCurrentScene() const;
    const DAVA::SelectableGroup& GetCurrentSelection() const;
    ModificationData* GetModificationData() const;

    void RecalculateTransformableSelectionField();

    void SetTransformType(DAVA::Selectable::TransformType);
    void SetTransformPivot(DAVA::Selectable::TransformPivot);
    void SetTransformInLocalCoordinates(bool value);

    void OnResetTransform();
    void OnLockTransform();
    void OnUnlockTransform();
    void OnCenterPivotPoint();
    void OnZeroPivotPoint();

    bool IsModificationEnabled() const;
    bool IsModificationArrowsEnabled() const;
    bool IsScaleModeOff() const;
    DAVA::String GetLabelX() const;
    DAVA::Any GetValueX() const;
    DAVA::Any GetValueY() const;
    DAVA::Any GetValueZ() const;
    void SetValueX(const DAVA::Any&);
    void SetValueY(const DAVA::Any&);
    void SetValueZ(const DAVA::Any&);

    ModificationInternalData* GetModificationInternalData() const;
    void ForceUpdateXYZControls();

private:
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
    DAVA::QtConnections connections;

    static const char* scaleModeOffField;
    static const char* manualModificationEnabledField;
    static const char* manualModificationArrowsEnabledField;
    static const char* xLabelField;
    static const char* xValueField;
    static const char* yValueField;
    static const char* zValueField;

    DAVA_VIRTUAL_REFLECTION(ModificationModule, DAVA::ClientModule);
};
