#pragma once

#include "Scene3D/Entity.h"
#include <QDialog>

namespace DAVA
{
class SceneEditor2;
} // namespace DAVA

class QualitySwitcher : public QDialog
{
    Q_OBJECT

public:
    static void ShowDialog();

protected:
    QualitySwitcher();
    ~QualitySwitcher();

    void ApplyTx();
    void ApplyMa();

    void UpdateEntitiesToQuality(DAVA::Entity* e);
    void UpdateParticlesToQuality();
    void ReloadEntityEmitters(DAVA::SceneEditor2* scene, DAVA::Entity* e);
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
    static QualitySwitcher* switcherDialog;
};
