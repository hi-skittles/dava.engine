#ifndef __LIST_MODEL_H__
#define __LIST_MODEL_H__

#include <QObject>
#include <QAbstractListModel>
#include <QFont>

class GuiApplicationManager;

class BranchesListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ListItemType
    {
        LIST_ITEM_NEWS,
        LIST_ITEM_FAVORITES,
        LIST_ITEM_BRANCH,
        LIST_ITEM_SEPARATOR
    };
    static const int DAVA_WIDGET_ROLE = Qt::UserRole + 1;
    BranchesListModel(const GuiApplicationManager* appManager_, QObject* parent = nullptr);
    void ClearItems();
    void AddItem(const QString& dataText, ListItemType type);
    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    ListItemType GetType(int row) const;

private:
    struct Item
    {
        QString text;
        QString dataText;
        ListItemType type;
    };

    QList<Item> items;
    QFont fontFavorites;
    const GuiApplicationManager* appManager;
};

Q_DECLARE_METATYPE(BranchesListModel::ListItemType);

#endif // __LIST_MODEL_H__
