#ifndef FILEPATHBROWSER_H
#define FILEPATHBROWSER_H


#include <QLabel>
#include <QCache>

#include "QtTools/LineEdit/LineEditEx.h"

class FilePathBrowser
: public LineEditEx
{
    Q_OBJECT

signals:
    void pathChanged(const QString& path);

public:
    enum eFileType
    {
        File,
        Folder
    };

    explicit FilePathBrowser(QWidget* parent = NULL);

    void SetHint(const QString& hint);
    void SetCurrentFolder(const QString& path);
    void SetPath(const QString& path);
    const QString& GetPath() const;

    void SetFilter(const QString& filter);

    void SetType(eFileType type);
    void AllowInvalidPath(bool allow);

    QSize sizeHint() const;

protected:
    QString CurrentBrowsePath();

private slots:
    void OnBrowse();
    void TryToAcceptPath();

private:
    void InitButtons();
    void TryToAcceptPath(const QString& path);

    QSize ButtonSizeHint(const QAction* action) const;
    void keyPressEvent(QKeyEvent* event);

    QPointer<QLabel> validIcon;
    QCache<bool, QPixmap> iconCache;

    bool allowInvalidPath;

    QString hintText;
    QString currentFolder;
    QString path;
    QString filter;

    eFileType type = eFileType::File;
};


#endif // FILEPATHBROWSER_H
