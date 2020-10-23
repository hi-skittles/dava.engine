#include "Test/Private/TestHelpers.h"
#include <TArc/Qt/QtString.h>

#include <Engine/Engine.h>
#include <FileSystem/FileSystem.h>
#include <UnitTests/UnitTests.h>
#include <QMainWindow>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>

void TestHelpers::CreateProjectFolder(const DAVA::FilePath& folderName)
{
    ClearTestFolder(); // to be sure that we have no any data at project folder that could stay in case of crash or stopping of debugging
    const DAVA::EngineContext* context = DAVA::GetEngineContext();
    DAVA::FileSystem* fs = context->fileSystem;

    fs->CreateDirectory(folderName, true);
}

void TestHelpers::ClearTestFolder()
{
    DAVA::FilePath folder(GetTestPath());

    DVASSERT(folder.IsDirectoryPathname());

    const DAVA::EngineContext* context = DAVA::GetEngineContext();
    DAVA::FileSystem* fs = context->fileSystem;
    fs->DeleteDirectoryFiles(folder, true);
    fs->DeleteDirectory(folder, true);
}

DAVA::FilePath TestHelpers::GetTestPath()
{
    return DAVA::FilePath("~doc:/Test/");
}

QAction* TestHelpers::FindActionInMenus(QWidget* window, const QString& menuName, const QString& actionNname)
{
    QMainWindow* mainWnd = qobject_cast<QMainWindow*>(window);
    TEST_VERIFY(mainWnd != nullptr);

    QMenuBar* menuBar = mainWnd->menuBar();
    QMenu* menu = menuBar->findChild<QMenu*>(menuName);

    QList<QAction*> actions = menu->actions();
    foreach (QAction* action, actions)
    {
        if (action->objectName() == actionNname)
        {
            return action;
        }
    }
    TEST_VERIFY(false);
    return nullptr;
}
