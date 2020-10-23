#ifndef __LIBRARY_FILESYSTEM_MODEL_H__
#define __LIBRARY_FILESYSTEM_MODEL_H__

#include <QFileSystemModel>
class LibraryFileSystemModel : public QFileSystemModel
{
    Q_OBJECT

    static const int ACCEPT_ROLE = Qt::UserRole + 5;

public:
    explicit LibraryFileSystemModel(QObject* parent = 0);

    void Load(const QString& pathname);

    void SetExtensionFilter(const QStringList& extensionFilter);

    void SetAccepted(const QModelIndex& index, bool accepted);
    bool IsAccepted(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    bool HasFilteredChildren(const QModelIndex& index);

signals:

    void ModelLoaded();

protected slots:

    void DirectoryLoaded(const QString& path);

protected:
    QMap<QString, QVariant> acceptionMap;

    uint loadingCounter;
};

#endif // __LIBRARY_FILESYSTEM_MODEL_H__
