#pragma once

#include <QWidget>
#include <QMimeData>
#include <qlineedit.h>
#include <qtoolbutton.h>
#include "SelectPathWidgetBase.h"

#include "DAVAEngine.h"
class SceneEditor2;

class SelectEntityPathWidget : public SelectPathWidgetBase
{
    Q_OBJECT

public:
    explicit SelectEntityPathWidget(QWidget* parent = 0, DAVA::String openDialoDefualtPath = "", DAVA::String relativPath = "");

    ~SelectEntityPathWidget();

    DAVA::Entity* GetOutputEntity(SceneEditor2*);

protected:
    void dragEnterEvent(QDragEnterEvent* event);

    DAVA::Entity* ConvertQMimeDataFromFilePath(SceneEditor2* sceneEditor = NULL);
    DAVA::Entity* ConvertFromMimeData(SceneEditor2* sceneEditor);

    void SetEntities(DAVA::Entity* entity, bool perfromRetain);

    DAVA::List<DAVA::Entity*> entitiesToHold;
};
