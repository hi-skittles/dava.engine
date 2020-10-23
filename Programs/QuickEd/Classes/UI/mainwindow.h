#pragma once

#include <TArc/DataProcessing/TArcDataNode.h>

#if defined(__DAVAENGINE_MACOS__)
#include <TArc/Utils/ShortcutChecker.h>
#endif //__DAVAENGINE_MACOS__
#include <TArc/Utils/QtDelayedExecutor.h>
#include <TArc/Utils/QtConnections.h>

#include <QMainWindow>
#include <QTranslator>

namespace Ui
{
class MainWindow;
}

class PackageWidget;
class StyleSheetInspectorWidget;

namespace DAVA
{
class ResultList;
class ContextAccessor;
class UI;
class OperationInvoker;
}

class QCheckBox;
class QActionGroup;
class QEvent;

class MainWindow : public QMainWindow, public DAVA::TrackedObject
{
    Q_OBJECT

public:
    class ProjectView;

    explicit MainWindow(DAVA::ContextAccessor* accessor, DAVA::UI* ui, DAVA::OperationInvoker* invoker, QWidget* parent = nullptr);

    ~MainWindow() override;

    void SetEditorTitle(const QString& editorTitle);

    ProjectView* GetProjectView() const;

signals:
    void EmulationModeChanged(bool emulationMode);

private:
    void SetProjectPath(const QString& projectPath);

    void ConnectActions();
    void InitEmulationMode();
    void SetupViewMenu();

    void UpdateWindowTitle();

    bool eventFilter(QObject* object, QEvent* event) override;

    std::unique_ptr<Ui::MainWindow> ui;

    QString editorTitle;
    QString projectPath;

    QCheckBox* emulationBox = nullptr;
    QActionGroup* backgroundActions = nullptr;

#if defined(__DAVAENGINE_MACOS__)
    DAVA::ShortcutChecker shortcutChecker;
#endif //__DAVAENGINE_MACOS__

    DAVA::QtDelayedExecutor delayedExecutor;
    DAVA::ContextAccessor* accessor = nullptr;
    DAVA::QtConnections connections;

    QTranslator translator;

    ProjectView* projectView = nullptr;
};
