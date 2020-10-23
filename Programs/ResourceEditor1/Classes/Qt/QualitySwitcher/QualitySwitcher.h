#pragma once

#include "Scene3D/Entity.h"
#include <QDialog>

class GlobalOperations;
class QualitySwitcher : public QDialog
{
    Q_OBJECT

public:
    static void ShowDialog(std::shared_ptr<GlobalOperations> globalOperations);

protected:
    QualitySwitcher(const std::shared_ptr<GlobalOperations>& globalOperations);
    ~QualitySwitcher();

    void ApplyTx();
    void ApplyMa();

    void UpdateEntitiesToQuality(DAVA::Entity* e);
    void UpdateParticlesToQuality();
    void ReloadEntityEmitters(SceneEditor2* scene, DAVA::Entity* e);
    void SetSettingsDirty(bool dirty);
    void ApplySettings();

protected slots:
    void OnSetSettingsDirty(int index);
    void OnOptionClick(bool);
    void OnParticlesTagsCloudChanged(const QString& text);

    void OnOkPressed();
    void OnCancelPressed();
    void OnApplyPressed();

private:
    bool settingsDirty = false;
    std::shared_ptr<GlobalOperations> globalOperations;
    static QualitySwitcher* switcherDialog;
};
