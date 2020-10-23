#include "SelectEntityPathWidget.h"
#include "Classes/Qt/Tools/MimeDataHelper/MimeDataHelper.h"

#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/StructureSystem.h>

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>

#include <QFileInfo>
#include <QKeyEvent>
#include <QUrl>
#include <QStyle>

SelectEntityPathWidget::SelectEntityPathWidget(QWidget* parent_, DAVA::Scene* scene_, DAVA::String openDialogDefaultPath_, DAVA::String relativePath_)
    : SelectPathWidgetBase(parent_, false, openDialogDefaultPath_, relativePath_, "Open Scene File", "Scene File (*.sc2)")
    , scene(scene_)
{
    allowedFormatsList.emplace_back(".sc2");
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

DAVA::RefPtr<DAVA::Entity> SelectEntityPathWidget::GetOutputEntity()
{
    return ConvertFromMimeData();
}

DAVA::RefPtr<DAVA::Entity> SelectEntityPathWidget::ConvertFromMimeData()
{
    using namespace DAVA;

    if (droppedObject.CanBeCastedTo<Entity>())
    {
        return RefPtr<Entity>::ConstructWithRetain(droppedObject.Cast<Entity>());
    }
    else if (selectedPath.isEmpty() == false)
    {
        return ConvertQMimeDataFromFilePath();
    }
    else
    {
        return RefPtr<Entity>();
    }
}

DAVA::RefPtr<DAVA::Entity> SelectEntityPathWidget::ConvertQMimeDataFromFilePath()
{
    using namespace DAVA;

    if (scene == nullptr || selectedPath.isEmpty() == true)
    {
        return RefPtr<Entity>();
    }

    FilePath filePath(selectedPath.toStdString());
    FileSystem* fs = GetEngineContext()->fileSystem;
    bool pathIsValid = false;
    for (const String& ext : allowedFormatsList)
    {
        if (filePath.IsEqualToExtension(ext) && fs->Exists(filePath))
        {
            pathIsValid = true;
            break;
        }
    }

    if (pathIsValid == false)
    {
        return RefPtr<Entity>();
    }

    Entity* entity = scene->GetSystem<StructureSystem>()->Load(filePath);
    return RefPtr<Entity>(entity);
}
