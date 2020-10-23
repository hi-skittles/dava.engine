#include "TileTexturePreviewWidgetItemDelegate.h"

#include <QLineEdit>

const QString TileTexturePreviewWidgetItemDelegate::TILE_COLOR_VALIDATE_REGEXP = "#{0,1}[A-F0-9]{6}";

TileTexturePreviewWidgetItemDelegate::TileTexturePreviewWidgetItemDelegate(QObject* parent /* = 0 */)
    : QItemDelegate(parent)
{
}

TileTexturePreviewWidgetItemDelegate::~TileTexturePreviewWidgetItemDelegate()
{
}

QWidget* TileTexturePreviewWidgetItemDelegate::createEditor(QWidget* parent,
                                                            const QStyleOptionViewItem& option,
                                                            const QModelIndex& index) const
{
    QWidget* widget = QItemDelegate::createEditor(parent, option, index);

    QLineEdit* edit = qobject_cast<QLineEdit*>(widget);
    if (edit != 0)
    {
        QRegExpValidator* validator = new QRegExpValidator();
        validator->setRegExp(QRegExp(TILE_COLOR_VALIDATE_REGEXP, Qt::CaseInsensitive));
        edit->setValidator(validator);
    }

    return widget;
}

void TileTexturePreviewWidgetItemDelegate::drawFocus(QPainter* painter,
                                                     const QStyleOptionViewItem& option,
                                                     const QRect& rect) const
{
}