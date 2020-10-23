#pragma once

#include <QStyledItemDelegate>

class QPainter;
class QStyleOptionViewItem;
class QModelIndex;

class SceneTreeItemDelegateV2 : public QStyledItemDelegate
{
public:
    SceneTreeItemDelegateV2(QObject* parent);
    ~SceneTreeItemDelegateV2() = default;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};