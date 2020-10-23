#include "Classes/SceneTree/Private/SceneTreeItemDelegateV2.h"
#include "Classes/SceneTree/Private/SceneTreeModelV2.h"
#include "Classes/SceneTree/Private/SceneTreeRoles.h"

#include <QPainter>
#include <QStyleOption>
#include <QAbstractItemModel>
#include <QVector>
#include <QIcon>
#include <QRect>

SceneTreeItemDelegateV2::SceneTreeItemDelegateV2(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void SceneTreeItemDelegateV2::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QVariant decoration = index.data(ToItemRoleCast(eSceneTreeRoles::RightAlignedDecorationRole));
    QVector<QIcon> icons;
    if (decoration.canConvert<QVector<QIcon>>())
    {
        icons = decoration.value<QVector<QIcon>>();
    }

    const int leftIconsMargin = 1;
    const int rightIconsMargin = 2;

    int iconsWidth = icons.size() * 16;
    if (iconsWidth > 0)
    {
        iconsWidth += (leftIconsMargin + rightIconsMargin);
    }

    QStyleOptionViewItem opt = option;
    opt.rect.setRight(opt.rect.right() - iconsWidth);

    QVariant foregroundAlpha = index.data(ToItemRoleCast(eSceneTreeRoles::ForegroundAlphaRole));
    if (foregroundAlpha.isValid() && foregroundAlpha.canConvert<int>())
    {
        QColor c = opt.palette.text().color();
        c.setAlpha(100);
        opt.palette.setColor(QPalette::Text, c);
    }

    QStyledItemDelegate::paint(painter, opt, index);

    if (icons.isEmpty() == true)
    {
        return;
    }

    QRect iconRect(opt.rect.right() + 1, opt.rect.top(), 16, 16);
    foreach (const QIcon& icon, icons)
    {
        icon.paint(painter, iconRect);
        QRect prevRect = iconRect;
        iconRect = QRect(prevRect.right(), prevRect.top(), 16, 16);
    }
}
