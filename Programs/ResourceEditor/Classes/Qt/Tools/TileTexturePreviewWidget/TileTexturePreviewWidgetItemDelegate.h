#ifndef __RESOURCEEDITORQT__TILETEXTUREPREVIEWWIDGETITEMDELEGATE__
#define __RESOURCEEDITORQT__TILETEXTUREPREVIEWWIDGETITEMDELEGATE__

#include <QWidget>
#include <QItemDelegate>

class TileTexturePreviewWidgetItemDelegate : public QItemDelegate
{
public:
    static const QString TILE_COLOR_VALIDATE_REGEXP;

    TileTexturePreviewWidgetItemDelegate(QObject* parent = 0);
    ~TileTexturePreviewWidgetItemDelegate();

    virtual QWidget* createEditor(QWidget* parent,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const;

protected:
    virtual void drawFocus(QPainter* painter,
                           const QStyleOptionViewItem& option,
                           const QRect& rect) const;
};

#endif /* defined(__RESOURCEEDITORQT__TILETEXTUREPREVIEWWIDGETITEMDELEGATE__) */
