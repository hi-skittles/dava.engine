#include "ResourceFilePropertyDelegate.h"
#include "PropertiesTreeItemDelegate.h"

#include "Application/QEGlobal.h"
#include "Modules/LegacySupportModule/Private/Project.h"
#include "Utils/MacOSSymLinkRestorer.h"
#include "Utils/QtDavaConvertion.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/PropertiesHolder.h>

#include <QtTools/FileDialogs/FileDialog.h>

#include <Engine/Engine.h>
#include <FileSystem/FileSystem.h>

#include <QAction>
#include <QLineEdit>
#include <QApplication>
#include <QMap>
#include <QDesktopServices>

using namespace DAVA;

ResourceFilePropertyDelegate::ResourceFilePropertyDelegate(
const QList<QString>& resourceExtensions_,
const QString& resourceSubDir_,
PropertiesTreeItemDelegate* delegate,
bool allowAnyExtension)
    : BasePropertyDelegate(delegate)
    , resourceExtensions(resourceExtensions_)
    , resourceSubDir(resourceSubDir_)
    , allowAnyExtension(allowAnyExtension)
{
}

ResourceFilePropertyDelegate::~ResourceFilePropertyDelegate()
{
}

QWidget* ResourceFilePropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem&, const QModelIndex&)
{
    DVASSERT(context.project != nullptr);
    DVASSERT(context.accessor != nullptr);
    accessor = context.accessor;
    project = context.project;
#if defined(__DAVAENGINE_MACOS__)
    QString directoryPath = project->GetResourceDirectory();
    symLinkRestorer = std::make_unique<MacOSSymLinkRestorer>(directoryPath);
#endif
    projectResourceDir = context.project->GetResourceDirectory();
    lineEdit = new QLineEdit(parent);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    connect(lineEdit, &QLineEdit::editingFinished, this, &ResourceFilePropertyDelegate::OnEditingFinished);
    connect(lineEdit, &QLineEdit::textChanged, this, &ResourceFilePropertyDelegate::OnTextChanged);
    return lineEdit;
}

void ResourceFilePropertyDelegate::setEditorData(QWidget*, const QModelIndex& index) const
{
    DAVA::Any variant = index.data(Qt::EditRole).value<DAVA::Any>();
    QString stringValue = StringToQString(variant.Get<FilePath>().GetStringValue());
    DVASSERT(!lineEdit.isNull());
    lineEdit->setText(stringValue);
}

bool ResourceFilePropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;

    DAVA::Any value = index.data(Qt::EditRole).value<DAVA::Any>();
    DVASSERT(!lineEdit.isNull());
    if (!lineEdit->text().isEmpty())
    {
        DAVA::FilePath absoluteFilePath = QStringToString(lineEdit->text());
        DAVA::FilePath frameworkFilePath = absoluteFilePath.GetFrameworkPath();
        value.Set(frameworkFilePath);
    }
    else
    {
        value.Set(DAVA::FilePath());
    }
    QVariant variant;
    variant.setValue<DAVA::Any>(value);

    return model->setData(index, variant, Qt::EditRole);
}

void ResourceFilePropertyDelegate::enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions)
{
    QAction* selectFileAction = new QAction(tr("..."), parent);
    selectFileAction->setToolTip(tr("Select resource file"));
    actions.push_back(selectFileAction);
    connect(selectFileAction, SIGNAL(triggered(bool)), this, SLOT(selectFileClicked()));

    QAction* gotoFileAction = new QAction(QIcon(":/Icons/search-icon.png"), tr("Go to file..."), parent);
    gotoFileAction->setToolTip(tr("Open resource file"));
    actions.push_back(gotoFileAction);
    connect(gotoFileAction, SIGNAL(triggered(bool)), this, SLOT(gotoFileClicked()));

    QAction* clearFileAction = new QAction(QIcon(":/Icons/editclear.png"), tr("clear"), parent);
    clearFileAction->setToolTip(tr("Clear resource file"));
    actions.push_back(clearFileAction);
    connect(clearFileAction, SIGNAL(triggered(bool)), this, SLOT(clearFileClicked()));

    BasePropertyDelegate::enumEditorActions(parent, index, actions);
}

void ResourceFilePropertyDelegate::selectFileClicked()
{
    using namespace DAVA;

    const String propertiesItemKey = "ResourceFilePropertyDelegate-" + resourceExtensions.join("-").toStdString();
    const String propertiesDirKey = "Dir";
    DVASSERT(!lineEdit.isNull());
    QWidget* editor = lineEdit->parentWidget();
    DVASSERT(editor != nullptr);

    DataContext* globalContext = accessor->GetGlobalContext();
    ProjectData* projectData = globalContext->GetData<ProjectData>();

    QString dir;
    QString pathText = lineEdit->text();
    if (!pathText.isEmpty())
    {
        FilePath filePath = QStringToString(pathText);
        dir = StringToQString(filePath.GetDirectory().GetAbsolutePathname());
    }
    else
    {
        PropertiesItem properties = projectData->CreatePropertiesNode(propertiesItemKey);
        String dirStr = properties.Get(propertiesDirKey, String(""));
        if (dirStr.empty())
        {
            dir = projectResourceDir + resourceSubDir;
            QFileInfo fi(dir);
            if (fi.exists() == false)
            {
                dir = projectResourceDir;
                fi = QFileInfo(dir);
                DVASSERT(fi.exists(), (dir.toStdString() + " not exists!").c_str());
            }
        }
        else
        {
            dir = QString::fromStdString(dirStr);
        }
    }

    QString filters;
    for (QString& filter : resourceExtensions)
    {
        filters += "*" + filter + " ";
    }
    QString filePathText = FileDialog::getOpenFileName(editor->parentWidget(), tr("Select resource file"), dir, filters);

    if (project)
    {
        filePathText = RestoreSymLinkInFilePath(filePathText);
    }

    if (!filePathText.isEmpty())
    {
        PropertiesItem properties = projectData->CreatePropertiesNode(propertiesItemKey);
        properties.Set(propertiesDirKey, filePathText.toStdString());

        FilePath absoluteFilePath = QStringToString(filePathText);
        FilePath frameworkFilePath = absoluteFilePath.GetFrameworkPath();
        lineEdit->setText(StringToQString(frameworkFilePath.GetStringValue()));

        BasePropertyDelegate::SetValueModified(editor, true);
        itemDelegate->emitCommitData(editor);
    }
}

void ResourceFilePropertyDelegate::clearFileClicked()
{
    DVASSERT(!lineEdit.isNull());
    QWidget* editor = lineEdit->parentWidget();
    DVASSERT(editor != nullptr);
    lineEdit->setText(QString(""));

    BasePropertyDelegate::SetValueModified(editor, true);
    itemDelegate->emitCommitData(editor);
}

void ResourceFilePropertyDelegate::gotoFileClicked()
{
    DVASSERT(!lineEdit.isNull());
    QWidget* editor = lineEdit->parentWidget();
    DVASSERT(editor != nullptr);
    QString pathText = lineEdit->text();

    DAVA::FilePath filePath = QStringToString(pathText);

    QString path = QString::fromStdString(filePath.GetAbsolutePathname());

    DVASSERT(itemDelegate->GetInvoker());
    if (path.endsWith(".yaml", Qt::CaseInsensitive))
    {
        itemDelegate->GetInvoker()->Invoke(QEGlobal::OpenDocumentByPath.ID, path);
    }
    else
    {
        QDesktopServices::openUrl(QUrl("file:///" + path, QUrl::TolerantMode));
    }
}

void ResourceFilePropertyDelegate::OnEditingFinished()
{
    DVASSERT(!lineEdit.isNull());
    if (!lineEdit->isModified())
    {
        return;
    }
    QWidget* editor = lineEdit->parentWidget();
    DVASSERT(editor != nullptr);
    const QString& text = lineEdit->text();
    if (!text.isEmpty() && !IsPathValid(text, allowAnyExtension))
    {
        return;
    }
    BasePropertyDelegate::SetValueModified(editor, lineEdit->isModified());
    itemDelegate->emitCommitData(editor);
    lineEdit->setModified(false);
}

void ResourceFilePropertyDelegate::OnTextChanged(const QString& text)
{
    QPalette palette(lineEdit->palette());
    QString textCopy(text);

    QColor globalTextColor = qApp->palette().color(QPalette::Text);
    QColor nextColor = IsPathValid(text, false) ? globalTextColor : Qt::red;
    palette.setColor(QPalette::Text, nextColor);
    lineEdit->setPalette(palette);
}

bool ResourceFilePropertyDelegate::IsPathValid(const QString& path, bool allowAnyExtension)
{
    QString fullPath = path;
    DAVA::FilePath filePath(QStringToString(fullPath));

    if (!filePath.IsEmpty() && !resourceExtensions.empty())
    {
        String ext = filePath.GetExtension();
        if (ext.empty() && !resourceExtensions.empty())
        {
            String resExt = QStringToString(resourceExtensions[0]);
            filePath.ReplaceExtension(resExt);
        }
        else if (!allowAnyExtension && !resourceExtensions.contains(StringToQString(ext)))
        {
            return false;
        }
    }

    DAVA::FileSystem* fileSystem = DAVA::Engine::Instance()->GetContext()->fileSystem;
    return fileSystem->Exists(filePath);
}

QString ResourceFilePropertyDelegate::RestoreSymLinkInFilePath(const QString& filePath) const
{
#if defined(__DAVAENGINE_MACOS__)
    return symLinkRestorer->RestoreSymLinkInFilePath(filePath);
#else
    return filePath;
#endif
}
