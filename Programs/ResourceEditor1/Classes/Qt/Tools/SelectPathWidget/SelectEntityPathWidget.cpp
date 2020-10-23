#include "SelectEntityPathWidget.h"
#include "Classes/DragNDropSupport/ReflectedMimeData.h"
#include "Classes/Qt/Scene/SceneEditor2.h"

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>

#include <QFileInfo>
#include <QKeyEvent>
#include <QUrl>
#include <QStyle>

SelectEntityPathWidget::SelectEntityPathWidget(QWidget* _parent, DAVA::String _openDialogDefualtPath, DAVA::String _relativPath)
    : SelectPathWidgetBase(_parent, false, _openDialogDefualtPath, _relativPath, "Open Scene File", "Scene File (*.sc2)")
{
    allowedFormatsList.push_back(".sc2");
}

SelectEntityPathWidget::~SelectEntityPathWidget()
{
    for (DAVA::Entity* item : entitiesToHold)
    {
        SafeRelease(item);
    }
}

void SelectEntityPathWidget::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (IsMimeDataCanBeDropped(mimeData) == false)
    {
        return;
    }

    event->setDropAction(Qt::LinkAction);

    bool isFormatSupported = true;

    if (event->mimeData()->hasFormat(MIME_URI_LIST_NAME))
    {
        isFormatSupported = false;
        DAVA::FilePath path(event->mimeData()->urls().first().toLocalFile().toStdString());
        for (const DAVA::String& item : allowedFormatsList)
        {
            if (path.IsEqualToExtension(item))
            {
                isFormatSupported = true;
                break;
            }
        }
    }
    if (isFormatSupported)
    {
        event->accept();
    }
}

DAVA::Entity* SelectEntityPathWidget::GetOutputEntity(SceneEditor2* editor)
{
    return ConvertFromMimeData(editor);
}

DAVA::Entity* SelectEntityPathWidget::ConvertFromMimeData(SceneEditor2* sceneEditor)
{
    if (droppedObject.CanBeCastedTo<DAVA::Entity>())
    {
        DAVA::Entity* result = droppedObject.Cast<DAVA::Entity>();
        SetEntities(result, true);
        return result;
    }
    else if (selectedPath.isEmpty() == false)
    {
        return ConvertQMimeDataFromFilePath(sceneEditor);
    }

    return nullptr;
}

DAVA::Entity* SelectEntityPathWidget::ConvertQMimeDataFromFilePath(SceneEditor2* sceneEditor)
{
    if (sceneEditor == nullptr || selectedPath.isEmpty() == true)
    {
        return nullptr;
    }

    DAVA::FilePath filePath(selectedPath.toStdString());
    DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
    bool pathIsValid = false;
    for (const DAVA::String& ext : allowedFormatsList)
    {
        if (filePath.IsEqualToExtension(ext) && fs->Exists(filePath))
        {
            pathIsValid = true;
            break;
        }
    }

    if (pathIsValid == false)
    {
        return nullptr;
    }

    DAVA::Entity* entity = sceneEditor->structureSystem->Load(filePath);

    // for just created entities no need to increase refCouner
    // it will be released in ~SelectEntityPathWidget()
    SetEntities(entity, false);
    return entity;
}

void SelectEntityPathWidget::SetEntities(DAVA::Entity* entity, bool perfromRetain)
{
    for (DAVA::Entity* item : entitiesToHold)
    {
        SafeRelease(item);
    }
    entitiesToHold.clear();
    entitiesToHold.push_back(entity);
    if (perfromRetain == true)
    {
        for (DAVA::Entity* item : entitiesToHold)
        {
            SafeRetain(item);
        }
    }
}
