#pragma once

#include "Base/Introspection.h"

#include <QDialog>

namespace Ui
{
class SceneValidationDialog;
}

namespace DAVA
{
class Scene;
}

class SceneValidationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SceneValidationDialog(DAVA::Scene* scene, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void OnSelectAllClicked(bool);
    void OnOptionToggled(int, bool);
    void Validate();
    void ShowConsole(bool checked);

private:
    void LoadOptions();
    void SaveOptions();

    bool AreAllOptionsSetTo(bool value);

    void DoMatrices();

private:
    DAVA::Scene* scene;
    Ui::SceneValidationDialog* ui;
};
