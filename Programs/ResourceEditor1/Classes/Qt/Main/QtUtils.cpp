#include "QtUtils.h"
#include "Deprecated/SceneValidator.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"

#include <QMessageBox>
#include <QToolButton>
#include <QFileInfo>

#include "mainwindow.h"

#include "CommandLine/CommandLineParser.h"

#include "QtTools/FileDialogs/FileDialog.h"
#include "QtTools/ConsoleWidget/PointerSerializer.h"

#include "DAVAEngine.h"

#include <Engine/Engine.h>
#include <DeviceManager/DeviceManager.h>
#include <Input/Keyboard.h>

#include <QProcess>

using namespace DAVA;

FilePath GetOpenFileName(const String& title, const FilePath& pathname, const String& filter)
{
    QString filePath = FileDialog::getOpenFileName(nullptr, QString(title.c_str()), QString(pathname.GetAbsolutePathname().c_str()),
                                                   QString(filter.c_str()));

    FilePath openedPathname(filePath.toStdString());
    if (!openedPathname.IsEmpty())
    {
        SceneValidator validator;
        ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
        if (data)
        {
            validator.SetPathForChecking(data->GetProjectPath());
        }

        if (validator.IsPathCorrectForProject(openedPathname) == false)
        {
            //Need to Show Error
            DAVA::Logger::Error("File(%s) was selected from incorect project.", openedPathname.GetAbsolutePathname().c_str());
            openedPathname = FilePath();
        }
    }

    return openedPathname;
}

String SizeInBytesToString(float32 size)
{
    String retString = "";

    if (1000000 < size)
    {
        retString = Format("%0.2f MB", size / (1024 * 1024));
    }
    else if (1000 < size)
    {
        retString = Format("%0.2f KB", size / 1024);
    }
    else
    {
        retString = Format("%d B", (int32)size);
    }

    return retString;
}

WideString SizeInBytesToWideString(float32 size)
{
    return UTF8Utils::EncodeToWideString(SizeInBytesToString(size));
}

bool IsKeyModificatorPressed(eInputElements key)
{
    Keyboard* keyboard = GetEngineContext()->deviceManager->GetKeyboard();
    return keyboard != nullptr && keyboard->GetKeyState(key).IsPressed();
}

bool IsKeyModificatorsPressed()
{
    return (IsKeyModificatorPressed(eInputElements::KB_LSHIFT) || IsKeyModificatorPressed(eInputElements::KB_LCTRL) || IsKeyModificatorPressed(eInputElements::KB_LALT));
}

int ShowQuestion(const String& header, const String& question, int buttons, int defaultButton)
{
    int answer = QMessageBox::question(NULL, QString::fromStdString(header), QString::fromStdString(question),
                                       (QMessageBox::StandardButton)buttons, (QMessageBox::StandardButton)defaultButton);

    return answer;
}

void ShowActionWithText(QToolBar* toolbar, QAction* action, bool showText)
{
    if (NULL != toolbar && NULL != action)
    {
        QToolButton* toolBnt = dynamic_cast<QToolButton*>(toolbar->widgetForAction(action));
        if (NULL != toolBnt)
        {
            toolBnt->setToolButtonStyle(showText ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
        }
    }
}

String ReplaceInString(const String& sourceString, const String& what, const String& on)
{
    String::size_type pos = sourceString.find(what);
    if (pos != String::npos)
    {
        String newString = sourceString;
        newString = newString.replace(pos, what.length(), on);
        return newString;
    }

    return sourceString;
}

void SaveSpriteToFile(Sprite* sprite, const FilePath& path)
{
    if (sprite)
    {
        SaveTextureToFile(sprite->GetTexture(), path);
    }
}

void SaveTextureToFile(Texture* texture, const FilePath& path)
{
    if (texture)
    {
        Image* img = texture->CreateImageFromMemory();
        SaveImageToFile(img, path);
        img->Release();
    }
}

void SaveImageToFile(Image* image, const FilePath& path)
{
    ImageSystem::Save(path, image);
}

DAVA::Texture* CreateSingleMipTexture(const DAVA::FilePath& imagePath)
{
    DAVA::ScopedPtr<DAVA::Image> image(DAVA::ImageSystem::LoadSingleMip(imagePath));

    DAVA::Texture* result = Texture::CreateFromData(image, false);
    DAVA::String baseName = imagePath.GetFilename();
    result->SetPathname(DAVA::Format("memoryfile_%s_%p", baseName.c_str(), result));

    return result;
}

const DAVA::FilePath& DefaultCursorPath()
{
    static const FilePath path = "~res:/ResourceEditor/LandscapeEditor/Tools/cursor/cursor.png";
    return path;
}
