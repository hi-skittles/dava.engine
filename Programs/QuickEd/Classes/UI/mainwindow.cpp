#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "Modules/LegacySupportModule/Private/Project.h"
#include "Render/Texture.h"

#include "Utils/QtDavaConvertion.h"

#include "QtTools/FileDialogs/FileDialog.h"
#include "QtTools/ReloadSprites/DialogReloadSprites.h"

#include "DebugTools/DebugTools.h"
#include "UI/ProjectView.h"

#include <TArc/Utils/Utils.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/ActionUtils.h>

#include <Base/Result.h>

#include <QMessageBox>
#include <QCheckBox>
#include <QKeyEvent>

MainWindow::MainWindow(DAVA::ContextAccessor* accessor_, DAVA::UI* tarcUi, DAVA::OperationInvoker* invoker, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow())
#if defined(__DAVAENGINE_MACOS__)
    , shortcutChecker(this)
#endif //__DAVAENGINE_MACOS__
    , accessor(accessor_)
{
    ui->setupUi(this);
    setObjectName("QuickEd"); //we need to support old names to save window settings

    ui->propertiesWidget->SetAccessor(accessor);
    ui->propertiesWidget->SetUI(tarcUi);
    ui->propertiesWidget->SetInvoker(invoker);

    setWindowIcon(QIcon(":/icon.ico"));
    DebugTools::ConnectToUI(ui.get());
    SetupViewMenu();

    projectView = new ProjectView(this);

    InitEmulationMode();
    ConnectActions();

    connect(projectView, &ProjectView::ProjectChanged, ui->propertiesWidget, &PropertiesWidget::SetProject);

    qApp->installEventFilter(this);

    translator.load(":/quicked_en");
    qApp->installTranslator(&translator);
}

MainWindow::~MainWindow() = default;

void MainWindow::SetEditorTitle(const QString& editorTitle_)
{
    editorTitle = editorTitle_;

    UpdateWindowTitle();
}

void MainWindow::SetProjectPath(const QString& projectPath_)
{
    projectPath = projectPath_;

    UpdateWindowTitle();
}

void MainWindow::ConnectActions()
{
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
}

void MainWindow::InitEmulationMode()
{
    emulationBox = new QCheckBox("Emulation", this);
    emulationBox->setLayoutDirection(Qt::RightToLeft);
    connect(emulationBox, &QCheckBox::toggled, this, &MainWindow::EmulationModeChanged);
    ui->toolBarGlobal->addSeparator();
    ui->toolBarGlobal->addWidget(emulationBox);
}

void MainWindow::SetupViewMenu()
{
    // Setup the common menu actions.
    QList<QAction*> dockWidgetToggleActions;
    dockWidgetToggleActions << ui->propertiesWidget->toggleViewAction()
                            << ui->toolBarGlobal->toggleViewAction();

    ui->Dock->insertActions(nullptr, dockWidgetToggleActions);
}

MainWindow::ProjectView* MainWindow::GetProjectView() const
{
    return projectView;
}

void MainWindow::UpdateWindowTitle()
{
    QString title;
    if (projectPath.isEmpty())
    {
        title = editorTitle;
    }
    else
    {
        title = QString("%1 | Project %2").arg(editorTitle).arg(projectPath);
    }
    setWindowTitle(title);
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
#if defined(__DAVAENGINE_MACOS__)
    if (QEvent::ShortcutOverride == event->type() && shortcutChecker.TryCallShortcut(static_cast<QKeyEvent*>(event)))
    {
        return true;
    }
#endif
    return false;
}
