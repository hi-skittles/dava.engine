#include "SelectPathWidgetBase.h"

#include <REPlatform/DataNodes/ReflectedMimeData.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>

#include <QtTools/FileDialogs/FileDialog.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Core/Deprecated.h>

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>
#include <Scene3D/Entity.h>

#include <QFileInfo>
#include <QKeyEvent>
#include <QUrl>
#include <QStyle>
#include <QMessageBox>

SelectPathWidgetBase::SelectPathWidgetBase(QWidget* _parent, bool _checkForProjectPath, DAVA::String _openDialogDefualtPath, DAVA::String _relativPath, DAVA::String _openFileDialogTitle, DAVA::String _fileFormatDescriotion)
    : QLineEdit(_parent)
    , checkForProjectPath(_checkForProjectPath)
{
    Init(_openDialogDefualtPath, _relativPath, _openFileDialogTitle, _fileFormatDescriotion);
}

SelectPathWidgetBase::~SelectPathWidgetBase()
{
    delete openButton;
    delete clearButton;
}

void SelectPathWidgetBase::Init(DAVA::String& _openDialogDefualtPath, DAVA::String& _relativPath, DAVA::String _openFileDialogTitle, DAVA::String _fileFormatFilter)
{
    setAcceptDrops(true);

    relativePath = DAVA::FilePath(_relativPath);
    openDialogDefaultPath = _openDialogDefualtPath;
    openFileDialogTitle = _openFileDialogTitle;
    fileFormatFilter = _fileFormatFilter;

    clearButton = CreateToolButton(":/QtIcons/ccancel.png");
    openButton = CreateToolButton(":/QtIcons/openscene.png");

    connect(clearButton, SIGNAL(clicked()), this, SLOT(EraseClicked()));
    connect(openButton, SIGNAL(clicked()), this, SLOT(OpenClicked()));
    connect(this, SIGNAL(editingFinished()), this, SLOT(acceptEditing()));
}

void SelectPathWidgetBase::resizeEvent(QResizeEvent*)
{
    QSize sz = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    clearButton->move(rect().right() - frameWidth - sz.width(), (rect().bottom() + 1 - sz.height()) / 2);

    QSize szOpenBtn = openButton->sizeHint();
    int offsetFromRight = clearButton->isVisible() ? sz.width() : 0;
    openButton->move(rect().right() - offsetFromRight - frameWidth - szOpenBtn.width(), (rect().bottom() + 1 - szOpenBtn.height()) / 2);
}

QToolButton* SelectPathWidgetBase::CreateToolButton(const DAVA::String& iconPath)
{
    QToolButton* retButton;

    retButton = new QToolButton(this);
    QIcon icon(iconPath.c_str());
    retButton->setIcon(icon);
    retButton->setCursor(Qt::ArrowCursor);
    retButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize msz = minimumSizeHint();
    setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(retButton->sizeHint().width() * 2 + frameWidth));
    setMinimumSize(qMax(msz.width(), retButton->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), retButton->sizeHint().height() + frameWidth * 2 + 2));

    return retButton;
}

void SelectPathWidgetBase::EraseWidget()
{
    setText(QString(""));
    selectedPath = QString();
    droppedObject = DAVA::Selectable();
}

void SelectPathWidgetBase::EraseClicked()
{
    EraseWidget();
}

void SelectPathWidgetBase::acceptEditing()
{
    this->setText(getText());
}

void SelectPathWidgetBase::setVisible(bool value)
{
    QLineEdit::setVisible(value);
    SelectPathWidgetBase::resizeEvent(NULL);
}

void SelectPathWidgetBase::OpenClicked()
{
    DAVA::FilePath presentPath(text().toStdString());
    DAVA::FilePath dialogString(openDialogDefaultPath);
    if (DAVA::FileSystem::Instance()->Exists(presentPath.GetDirectory())) //check if file text box clean
    {
        dialogString = presentPath.GetDirectory();
    }
    this->blockSignals(true);
    DAVA::String retString = FileDialog::getOpenFileName(this, openFileDialogTitle.c_str(), QString(dialogString.GetAbsolutePathname().c_str()), fileFormatFilter.c_str()).toStdString();
    this->blockSignals(false);

    if (retString.empty())
    {
        return;
    }

    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data != nullptr);
    DAVA::String projectPath = data->GetProjectPath().GetAbsolutePathname();

    if (checkForProjectPath && DAVA::String::npos == retString.find(projectPath))
    {
        QMessageBox::warning(NULL, "Wrong file selected", QString(DAVA::Format("Path %s doesn't belong to project.", retString.c_str()).c_str()), QMessageBox::Ok);
        return;
    }

    HandlePathSelected(retString);
}

void SelectPathWidgetBase::HandlePathSelected(const DAVA::String& name)
{
    DAVA::FilePath fullPath(name);

    DVASSERT(DAVA::FileSystem::Instance()->Exists(fullPath));

    setText(name);
    selectedPath = QString::fromStdString(name);
}

void SelectPathWidgetBase::setText(const QString& filePath)
{
    QLineEdit::setText(filePath);
    setToolTip(filePath);
    emit PathSelected(filePath.toStdString());
}

void SelectPathWidgetBase::setText(const DAVA::String& filePath)
{
    SelectPathWidgetBase::setText(QString(filePath.c_str()));
}

DAVA::String SelectPathWidgetBase::getText()
{
    return text().toStdString();
}

DAVA::String SelectPathWidgetBase::ConvertToRelativPath(const DAVA::String& path)
{
    DAVA::FilePath fullPath(path);
    if (DAVA::FileSystem::Instance()->Exists(fullPath))
    {
        return fullPath.GetRelativePathname(relativePath);
    }
    else
    {
        return path;
    }
}

void SelectPathWidgetBase::dropEvent(QDropEvent* event)
{
    const QMimeData* sendedMimeData = event->mimeData();
    const DAVA::ReflectedMimeData* reflMimeData = dynamic_cast<const DAVA::ReflectedMimeData*>(sendedMimeData);

    selectedPath = QString();
    droppedObject = DAVA::Selectable();

    if (sendedMimeData->hasFormat(MIME_URI_LIST_NAME) == true)
    {
        QList<QUrl> droppedUrls = sendedMimeData->urls();
        if (droppedUrls.isEmpty() == false)
        {
            selectedPath = droppedUrls.front().toLocalFile();
            DAVA::String droppedText = selectedPath.toStdString();
            DAVA::FilePath filePath(droppedText);
            if (DAVA::GetEngineContext()->fileSystem->Exists(filePath)) // check is it item form scene tree or file system
            {
                setText(filePath.GetAbsolutePathname());
            }
            else
            {
                setText(droppedText);
            }
        }
    }
    else if (reflMimeData != nullptr)
    {
        DAVA::Vector<DAVA::Entity*> entities = reflMimeData->GetObjects<DAVA::Entity>();
        if (entities.empty() == false)
        {
            DAVA::Entity* entity = entities.front();
            droppedObject = DAVA::Selectable(entity);
            setText(DAVA::String(entity->GetName().c_str()));
        }
    }

    event->setDropAction(Qt::LinkAction);
    event->accept();
}

void SelectPathWidgetBase::dragEnterEvent(QDragEnterEvent* event)
{
    event->setDropAction(Qt::LinkAction);
    if (IsMimeDataCanBeDropped(event->mimeData()) == true)
    {
        event->accept();
    }
}

void SelectPathWidgetBase::dragMoveEvent(QDragMoveEvent* event)
{
    event->setDropAction(Qt::LinkAction);
    event->accept();
}

bool SelectPathWidgetBase::IsMimeDataCanBeDropped(const QMimeData* data) const
{
    if (data->hasFormat(MIME_URI_LIST_NAME) == true)
    {
        return true;
    }

    const DAVA::ReflectedMimeData* reflMimeData = dynamic_cast<const DAVA::ReflectedMimeData*>(data);
    if (reflMimeData == nullptr)
    {
        return false;
    }

    return reflMimeData->HasObjects<DAVA::Entity>();
}

bool SelectPathWidgetBase::IsOpenButtonVisible() const
{
    return openButton->isVisible();
}

void SelectPathWidgetBase::SetOpenButtonVisible(bool value)
{
    openButton->setVisible(value);
}

bool SelectPathWidgetBase::IsClearButtonVisible() const
{
    return clearButton->isVisible();
}

void SelectPathWidgetBase::SetClearButtonVisible(bool value)
{
    clearButton->setVisible(value);
}

DAVA::String SelectPathWidgetBase::GetOpenDialogDefaultPath() const
{
    return openDialogDefaultPath;
}

void SelectPathWidgetBase::SetOpenDialogDefaultPath(const DAVA::FilePath& path)
{
    openDialogDefaultPath = path.GetAbsolutePathname();
}

DAVA::String SelectPathWidgetBase::GetFileFormatFilter() const
{
    return fileFormatFilter;
}

void SelectPathWidgetBase::SetFileFormatFilter(const DAVA::String& filter)
{
    fileFormatFilter = filter;
}

const QString SelectPathWidgetBase::MIME_URI_LIST_NAME = QStringLiteral("text/uri-list");