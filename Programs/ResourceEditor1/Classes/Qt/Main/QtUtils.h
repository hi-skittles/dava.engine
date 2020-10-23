#pragma once

#include "DAVAEngine.h"
#include <QString>
#include <QColor>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>

DAVA::FilePath GetOpenFileName(const DAVA::String& title, const DAVA::FilePath& pathname, const DAVA::String& filter);

void ShowActionWithText(QToolBar* toolbar, QAction* action, bool showText);

DAVA::WideString SizeInBytesToWideString(DAVA::float32 size);
DAVA::String SizeInBytesToString(DAVA::float32 size);

bool IsKeyModificatorPressed(DAVA::eInputElements key);
bool IsKeyModificatorsPressed();

enum eMessageBoxFlags
{
    MB_FLAG_YES = QMessageBox::Yes,
    MB_FLAG_NO = QMessageBox::No,
    MB_FLAG_CANCEL = QMessageBox::Cancel
};

int ShowQuestion(const DAVA::String& header, const DAVA::String& question, int buttons, int defaultButton);

#ifdef __DAVAENGINE_WIN32__
const Qt::WindowFlags WINDOWFLAG_ON_TOP_OF_APPLICATION = Qt::Window;
#else
const Qt::WindowFlags WINDOWFLAG_ON_TOP_OF_APPLICATION = Qt::Tool;
#endif

DAVA::String ReplaceInString(const DAVA::String& sourceString, const DAVA::String& what, const DAVA::String& on);

// Method for debugging. Save image to file
void SaveSpriteToFile(DAVA::Sprite* sprite, const DAVA::FilePath& path);
void SaveTextureToFile(DAVA::Texture* texture, const DAVA::FilePath& path);
void SaveImageToFile(DAVA::Image* image, const DAVA::FilePath& path);

DAVA::Texture* CreateSingleMipTexture(const DAVA::FilePath& pngPathname);
const DAVA::FilePath& DefaultCursorPath();
