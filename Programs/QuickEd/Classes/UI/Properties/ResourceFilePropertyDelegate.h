#pragma once

#include "BasePropertyDelegate.h"

class Project;
#if defined(__DAVAENGINE_MACOS__)
class MacOSSymLinkRestorer;
#endif //__DAVAENGINE_MACOS__

class ResourceFilePropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit ResourceFilePropertyDelegate(const QList<QString>& extensions, const QString& resourceSubDir, PropertiesTreeItemDelegate* delegate, bool allowAnyExtension);
    ~ResourceFilePropertyDelegate();

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions) override;

private slots:
    void selectFileClicked();
    void clearFileClicked();
    void gotoFileClicked();
    void OnEditingFinished();
    void OnTextChanged(const QString& text);

private:
    bool IsPathValid(const QString& path, bool allowAnyExtension);
    QString RestoreSymLinkInFilePath(const QString& filePath) const;

    QPointer<QLineEdit> lineEdit = nullptr;
    const Project* project = nullptr;

    QList<QString> resourceExtensions;
    QString projectResourceDir;
    QString resourceSubDir;
#if defined(__DAVAENGINE_MACOS__)
    std::unique_ptr<MacOSSymLinkRestorer> symLinkRestorer;
#endif
    bool allowAnyExtension = false;
};
