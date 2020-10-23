#ifndef DEBUG_VERSION_INFO_WIDGET_H
#define DEBUG_VERSION_INFO_WIDGET_H

#include <QWidget>
#include <QScopedPointer>

#include "Scene3D/SceneFile/VersionInfo.h"

namespace Ui
{
class VersionInfoWidget;
} // namespace Ui

class VersionInfoWidget
: public QWidget
{
    Q_OBJECT

public:
    explicit VersionInfoWidget(QWidget* parent);
    ~VersionInfoWidget();

private slots:
    void OnTemplateChanged(const QString& templateName);
    void OnAddTemplate();
    void OnRemoveTemplate();
    void OnSelectionChanged();

private:
    void FillTemplateList();
    void Reset();

    QScopedPointer<Ui::VersionInfoWidget> ui;

    typedef QPair<QString, DAVA::VersionInfo::VersionMap> VersionTemplate;
    typedef QList<VersionTemplate> TemplateList;
    TemplateList versions;
};


#endif
