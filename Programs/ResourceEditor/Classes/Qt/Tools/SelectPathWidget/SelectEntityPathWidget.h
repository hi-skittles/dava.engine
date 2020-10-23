#pragma once

#include "SelectPathWidgetBase.h"

#include <QWidget>
#include <QMimeData>
#include <QLineEdit>
#include <QToolButton>

#include "DAVAEngine.h"
namespace DAVA
{
class SceneEditor2;
} // namespace DAVA

class SelectEntityPathWidget : public SelectPathWidgetBase
{
    Q_OBJECT

public:
    explicit SelectEntityPathWidget(QWidget* parent, DAVA::Scene* scene, DAVA::String openDialogDefaultPath = "", DAVA::String relativePath = "");

    DAVA::RefPtr<DAVA::Entity> GetOutputEntity();

private:
    void dragEnterEvent(QDragEnterEvent* event) override;

    DAVA::RefPtr<DAVA::Entity> ConvertQMimeDataFromFilePath();
    DAVA::RefPtr<DAVA::Entity> ConvertFromMimeData();

    DAVA::Scene* scene = nullptr;
};
