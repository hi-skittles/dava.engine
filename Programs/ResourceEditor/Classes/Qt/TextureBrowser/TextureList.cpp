#include "TextureList.h"

TextureList::TextureList(QWidget* parent /* = 0 */)
    : QListView(parent)
{
}

void TextureList::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QListView::selectionChanged(selected, deselected);
    QModelIndex selectedIndex;
    if (!selected.isEmpty())
        selectedIndex = selected.begin()->topLeft();

    emit this->selected(selectedIndex);
}
