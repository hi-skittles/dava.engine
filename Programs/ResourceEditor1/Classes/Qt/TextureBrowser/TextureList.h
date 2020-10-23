#ifndef __TEXTURE_LIST_H__
#define __TEXTURE_LIST_H__

#include <QListView>

class TextureList : public QListView
{
    Q_OBJECT

public:
    TextureList(QWidget* parent = 0);

signals:
    void selected(const QModelIndex& index);

protected:
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
};

#endif // __TEXTURE_LIST_H__
