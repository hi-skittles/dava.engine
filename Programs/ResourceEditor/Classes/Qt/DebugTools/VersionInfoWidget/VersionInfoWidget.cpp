#include "VersionInfoWidget.h"

#include "ui_VersionInfoWidget.h"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QIntValidator>

namespace
{
enum TemplateInfoRoles
{
    eVersionRole = Qt::UserRole + 1,
    eTagTextRole,
    eTagRevisionRole,
};
}

VersionInfoWidget::VersionInfoWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::VersionInfoWidget())
{
    ui->setupUi(this);

    ui->ver->setValidator(new QIntValidator(0, 10000, this));
    ui->revision->setValidator(new QIntValidator(0, 10000, this));

    QStandardItemModel* model = new QStandardItemModel(this);
    ui->view->setModel(model);

    FillTemplateList();
    ui->tagTemplate->setCurrentIndex(-1);
    Reset();

    connect(ui->tagTemplate, SIGNAL(currentIndexChanged(const QString&)), SLOT(OnTemplateChanged(const QString&)));
    connect(ui->view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), SLOT(OnSelectionChanged()));
    connect(ui->add, SIGNAL(clicked()), SLOT(OnAddTemplate()));
    connect(ui->remove, SIGNAL(clicked()), SLOT(OnRemoveTemplate()));

#ifndef USER_VERSIONING_DEBUG_FEATURES
    ui->add->setEnabled(false);
    ui->remove->setEnabled(false);
    ui->tagTemplate->setEnabled(false);
#endif
}

VersionInfoWidget::~VersionInfoWidget()
{
}

void VersionInfoWidget::FillTemplateList()
{
    versions.clear();

#ifdef USER_VERSIONING_DEBUG_FEATURES

    {
        DAVA::VersionInfo::VersionMap defaultVersion = DAVA::VersionInfo::Instance()->GetDefaultVersionHistory();
        versions << VersionTemplate("Default", defaultVersion);
    }

    {
        DAVA::VersionInfo::VersionMap versionMap;

        DAVA::VersionInfo::SceneVersion v14;
        v14.version = 14;
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("vegetation", 1));
        DAVA::VersionInfo::AddVersion(versionMap, v14);

        versions << VersionTemplate("Case 1", versionMap);
    }

    {
        DAVA::VersionInfo::VersionMap versionMap;

        DAVA::VersionInfo::SceneVersion v14;
        v14.version = 14;
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("sky", 2));
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("vegetation", 1));
        DAVA::VersionInfo::AddVersion(versionMap, v14);

        versions << VersionTemplate("Case 2", versionMap);
    }

    {
        DAVA::VersionInfo::VersionMap versionMap;

        DAVA::VersionInfo::SceneVersion v14;
        v14.version = 14;
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("sky", 2));
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("vegetation", 1));
        DAVA::VersionInfo::AddVersion(versionMap, v14);

        DAVA::VersionInfo::SceneVersion v15;
        v15.version = 15;
        v15.tags.insert(DAVA::VersionInfo::TagsMap::value_type("fog", 1));
        DAVA::VersionInfo::AddVersion(versionMap, v15);

        versions << VersionTemplate("Case 3", versionMap);
    }

    {
        DAVA::VersionInfo::VersionMap versionMap;

        DAVA::VersionInfo::SceneVersion v14;
        v14.version = 14;
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("sky", 2));
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("vegetation", 1));
        v14.tags.insert(DAVA::VersionInfo::TagsMap::value_type("occlusion", 1));
        DAVA::VersionInfo::AddVersion(versionMap, v14);

        DAVA::VersionInfo::SceneVersion v15;
        v15.version = 15;
        v15.tags.insert(DAVA::VersionInfo::TagsMap::value_type("fog", 1));
        DAVA::VersionInfo::AddVersion(versionMap, v15);

        versions << VersionTemplate("Case 4", versionMap);
    }

#endif

    for (int i = 0; i < versions.size(); i++)
    {
        const QString& text = versions.at(i).first;
        ui->tagTemplate->addItem(text);
    }
}

void VersionInfoWidget::Reset()
{
    const DAVA::VersionInfo::VersionMap& versionMap = DAVA::VersionInfo::Instance()->Versions();

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->view->model());
    DVASSERT(model);

    model->clear();
    for (auto itVersion = versionMap.begin(); itVersion != versionMap.end(); ++itVersion)
    {
        const DAVA::VersionInfo::SceneVersion& version = itVersion->second;

        QStandardItem* verItem = new QStandardItem();
        verItem->setData(version.version, Qt::DisplayRole);
        verItem->setData(version.version, eVersionRole);

        for (auto itTags = version.tags.begin(); itTags != version.tags.end(); ++itTags)
        {
            const QString tag = itTags->first.c_str();
            const int rev = itTags->second;
            const QString tagText = QString("%1 %2").arg(tag).arg(rev);
            QStandardItem* tagItem = new QStandardItem();
            tagItem->setData(tagText, Qt::DisplayRole);
            tagItem->setData(version.version, eVersionRole);
            tagItem->setData(tag, eTagTextRole);
            tagItem->setData(rev, eTagRevisionRole);

            verItem->appendRow(tagItem);
        }
        model->appendRow(verItem);
    }

    ui->view->expandAll();

    ui->version->setText(QString::number(DAVA::VersionInfo::Instance()->GetCurrentVersion().version));
}

void VersionInfoWidget::OnTemplateChanged(const QString& templateName)
{
#ifdef USER_VERSIONING_DEBUG_FEATURES
    for (int i = 0; i < versions.size(); i++)
    {
        const QString& text = versions.at(i).first;
        if (templateName == text)
        {
            const DAVA::VersionInfo::VersionMap& versionMap = versions.value(i).second;
            DVASSERT(!versionMap.empty());
            DAVA::VersionInfo::Instance()->Versions() = versionMap;
            Reset();
            break;
        }
    }
#endif
}

void VersionInfoWidget::OnAddTemplate()
{
#ifdef USER_VERSIONING_DEBUG_FEATURES
    if (ui->ver->text().isEmpty())
        return;

    const uint ver = ui->ver->text().toUInt();
    const QString tag = ui->tag->text();
    const uint rev = ui->revision->text().toUInt();
    const bool isRoot = ui->tag->text().isEmpty() || ui->revision->text().isEmpty();

    DAVA::VersionInfo::VersionMap& versionMap = DAVA::VersionInfo::Instance()->Versions();

    auto itVersion = versionMap.find(ver);
    if (itVersion == versionMap.end())
    {
        DAVA::VersionInfo::SceneVersion version;
        version.version = ver;
        DAVA::VersionInfo::AddVersion(versionMap, version);
    }

    if (!isRoot)
    {
        DAVA::VersionInfo::SceneVersion& sceneVer = versionMap[ver];
        sceneVer.tags.insert(DAVA::VersionInfo::TagsMap::value_type(tag.toStdString(), rev));
    }

    Reset();
#endif
}

void VersionInfoWidget::OnRemoveTemplate()
{
#ifdef USER_VERSIONING_DEBUG_FEATURES
    DAVA::VersionInfo::VersionMap& versionMap = DAVA::VersionInfo::Instance()->Versions();
    const uint ver = ui->ver->text().toUInt();
    const QString tag = ui->tag->text();
    const uint rev = ui->revision->text().toUInt();
    const bool isRoot = ui->tag->text().isEmpty() || ui->revision->text().isEmpty();

    auto itVersion = versionMap.find(ver);
    if (itVersion == versionMap.end())
    {
        return;
    }

    if (isRoot)
    {
        if (versionMap.size() == 1) // Don't erase last version
        {
            return;
        }
        versionMap.erase(itVersion);
    }
    else
    {
        DAVA::VersionInfo::SceneVersion& sceneVer = itVersion->second;
        auto itTag = sceneVer.tags.find(tag.toStdString());
        if (itTag != sceneVer.tags.end() && itTag->second == rev)
        {
            sceneVer.tags.erase(itTag);
        }
    }

    Reset();
#endif
}

void VersionInfoWidget::OnSelectionChanged()
{
    QItemSelectionModel* selection = ui->view->selectionModel();
    const bool hasSelection = selection->hasSelection();

    if (!hasSelection)
    {
        ui->ver->setText(QString());
        ui->tag->setText(QString());
        ui->revision->setText(QString());
        return;
    }

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->view->model());
    DVASSERT(model);
    QStandardItem* item = model->itemFromIndex(selection->currentIndex());
    DVASSERT(item);

    const uint ver = item->data(eVersionRole).toUInt();
    const QString tag = item->data(eTagTextRole).toString();
    const uint rev = item->data(eTagRevisionRole).toUInt();
    //const bool t1 = item->data(eTagTextRole).isValid();
    //const bool t2 = item->data(eTagRevisionRole).isValid();
    const bool isRoot = !item->data(eTagTextRole).isValid() || !item->data(eTagRevisionRole).isValid();

    ui->ver->setText(QString::number(ver));
    if (!isRoot)
    {
        ui->tag->setText(tag);
        ui->revision->setText(QString::number(rev));
    }
    else
    {
        ui->tag->setText(QString());
        ui->revision->setText(QString());
    }
}
