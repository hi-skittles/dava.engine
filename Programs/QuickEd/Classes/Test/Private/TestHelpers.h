#pragma once

#include <FileSystem/FilePath.h>

class QString;
class QAction;
class QWidget;

namespace TestHelpers
{
void CreateProjectFolder(const DAVA::FilePath& folder);
void ClearTestFolder();
DAVA::FilePath GetTestPath();

QAction* FindActionInMenus(QWidget* window, const QString& menuName, const QString& actionNname);
} //nemspace TestHelpers
