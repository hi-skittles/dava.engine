#ifndef __QT_PROPERY_ITEM_DELEGATE_H__
#define __QT_PROPERY_ITEM_DELEGATE_H__

#include <QStyledItemDelegate>
#include <QPointer>
#include <QWidget>
#include <QAbstractItemView>
#include "QtPropertyData.h"

class QtPropertyData;
class QtPropertyModel;

class QtPropertyItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    QtPropertyItemDelegate(QAbstractItemView* view, QtPropertyModel* model, QWidget* parent = 0);
    virtual ~QtPropertyItemDelegate();

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    void showButtons(QtPropertyData* data);
    void invalidateButtons();

public slots:
    bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index);

private:
    bool eventFilter(QObject* obj, QEvent* event);
    void drawOptionalButtons(QPainter* painter, QStyleOptionViewItem& option, const QModelIndex& index) const;
    void showOptionalButtons(QtPropertyData* data);
    void hideButtons();
    void DrawButton(QPainter* painter, QStyleOptionViewItem& opt, QtPropertyToolButton* btn) const;

    QtPropertyModel* model = nullptr;
    QtPropertyData* lastHoverData = nullptr;
    DAVA::Vector<QPointer<QtPropertyToolButton>> visibleButtons;
    QPointer<QAbstractItemView> view;
    mutable QPointer<QWidget> activeEditor;
    mutable bool editorDataWasSet;
    const int buttonSpacing = 1;
};

#endif // __QT_PROPERY_ITEM_DELEGATE_H__
