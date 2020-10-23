#pragma once

#include <memory>

#include <QWidget>
#include <QScopedPointer>
#include <QVariant>
#include <QMap>
#include <QPointer>
#include <QStringListModel>

namespace Ui
{
class RunActionEventWidget;
}

namespace DAVA
{
class Any;

namespace TArc
{
class FieldBinder;
}
}

class SceneEditor2;
class SelectableGroup;

class RunActionEventWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RunActionEventWidget(QWidget* parent = NULL);
    ~RunActionEventWidget() override;

private:
    QScopedPointer<Ui::RunActionEventWidget> ui;
    QMap<int, int> editorIdMap;
    QPointer<QStringListModel> autocompleteModel;
    SceneEditor2* scene = nullptr;

private slots:
    void OnTypeChanged();
    void OnInvoke();
    void sceneActivated(SceneEditor2* scene);
    void sceneDeactivated(SceneEditor2* scene);

private:
    void OnSelectionChanged(const DAVA::Any& selection);
    std::unique_ptr<DAVA::TArc::FieldBinder> selectionFieldBinder;
};
