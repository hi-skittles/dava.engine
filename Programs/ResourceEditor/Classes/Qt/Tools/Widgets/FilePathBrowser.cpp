#include "FilePathBrowser.h"

#include <QAction>
#include <QAbstractButton>
#include <QDebug>
#include <QBoxLayout>
#include <QFileInfo>
#include <QKeyEvent>

#include <QtTools/FileDialogs/FileDialog.h>
#include <TArc/Utils/Utils.h>

namespace
{
const QSize cButtonSize = QSize(24, 24);
}

FilePathBrowser::FilePathBrowser(QWidget* parent)
    : LineEditEx(parent)
    , iconCache(2)
    , allowInvalidPath(false)
{
    InitButtons();

    QBoxLayout* l = qobject_cast<QBoxLayout*>(layout());
    if (l != NULL)
    {
        l->setContentsMargins(2, 2, 2, 2);
    }

    connect(this, SIGNAL(textUpdated(const QString&)), SLOT(TryToAcceptPath()));
}

void FilePathBrowser::SetHint(const QString& hint)
{
    hintText = hint;
    setPlaceholderText(hintText);
}

void FilePathBrowser::SetCurrentFolder(const QString& _path)
{
    currentFolder = _path;
}

void FilePathBrowser::AllowInvalidPath(bool allow)
{
    allowInvalidPath = allow;
}

void FilePathBrowser::SetPath(const QString& _path)
{
    path = _path;
    setText(path);
    setToolTip(path);

    if (type == eFileType::Folder)
    {
        SetCurrentFolder(path);
    }
}

const QString& FilePathBrowser::GetPath() const
{
    return path;
}

void FilePathBrowser::SetFilter(const QString& _filter)
{
    filter = _filter;
}

QSize FilePathBrowser::sizeHint() const
{
    QSize hint = LineEditEx::sizeHint();
    const bool hasActions = !actions().isEmpty();

    if (hasActions)
    {
        hint.rheight() = cButtonSize.height();

        QBoxLayout* l = qobject_cast<QBoxLayout*>(layout());
        if (l != NULL)
        {
            int top, left, right, bottom;
            l->getContentsMargins(&left, &top, &right, &bottom);
            hint.rheight() += top + bottom;
        }
    }

    return hint;
}

QString FilePathBrowser::CurrentBrowsePath()
{
    const QFileInfo pathInfo(text());
    const QFileInfo defaultInfo(currentFolder);

    if (pathInfo.isFile())
        return path;

    if (defaultInfo.isDir())
        return currentFolder;

    return QString();
}

void FilePathBrowser::OnBrowse()
{
    QString newPath;
    if (type == eFileType::File)
    {
        newPath = FileDialog::getOpenFileName(this, hintText, CurrentBrowsePath(), filter, nullptr, 0);
    }
    else
    {
        newPath = FileDialog::getExistingDirectory(this, hintText, CurrentBrowsePath());
    }

    if (!newPath.isEmpty())
    {
        TryToAcceptPath(newPath);
    }
}

void FilePathBrowser::TryToAcceptPath()
{
    QString newPath(text());
    QFileInfo newInfo(newPath);
    const bool isValid = (type == eFileType::File ? newInfo.isFile() : newInfo.isDir());

    // Icon
    QPixmap* pix = NULL;
    if (iconCache.contains(isValid))
    {
        pix = iconCache.object(isValid);
    }
    else
    {
        const QString uri = isValid ? ":/QtIcons/accept_button.png" : ":/QtIcons/prohibition_button.png";
        pix = new QPixmap(uri);
        iconCache.insert(isValid, pix);
    }
    validIcon->setPixmap(*pix);

    // Tooltip
    const QString toolTip = (type == eFileType::File ?
                             (isValid ? "File exists" : "File doesn't exists") :
                             (isValid ? "Folder exists" : "Folder doesn't exists"));
    validIcon->setToolTip(toolTip);

    if (isValid || allowInvalidPath)
    {
        SetPath(newPath);
        emit pathChanged(newPath);
    }
}

void FilePathBrowser::InitButtons()
{
    validIcon = new QLabel();
    validIcon->setFixedSize(ButtonSizeHint(NULL));
    validIcon->setAlignment(Qt::AlignCenter);
    AddCustomWidget(validIcon);

    QAction* browse = new QAction(this);
    browse->setToolTip("Browse...");
    browse->setIcon(DAVA::SharedIcon(":/QtIcons/openscene.png"));
    connect(browse, SIGNAL(triggered()), SLOT(OnBrowse()));
    addAction(browse);
}

void FilePathBrowser::TryToAcceptPath(const QString& _path)
{
    QFileInfo newInfo(_path);

    if (allowInvalidPath || (newInfo.isFile() && type == eFileType::File) || (newInfo.isDir() && type == eFileType::Folder))
    {
        SetPath(_path);
        emit pathChanged(_path);
    }
}

QSize FilePathBrowser::ButtonSizeHint(const QAction* action) const
{
    Q_UNUSED(action);
    return cButtonSize;
}

void FilePathBrowser::keyPressEvent(QKeyEvent* event)
{
    LineEditEx::keyPressEvent(event);

    switch (event->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        event->accept();
        break;

    default:
        break;
    }
}

void FilePathBrowser::SetType(eFileType type_)
{
    type = type_;
}
