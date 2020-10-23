#include <QToolTip>
#include <QHelpEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QTreeView>
#include <QScrollBar>

#include "QtPropertyItemDelegate.h"
#include "QtPropertyModel.h"
#include "QtPropertyData.h"
#include "QtPropertyData/QtPropertyDataDavaVariant.h"

QtPropertyItemDelegate::QtPropertyItemDelegate(QAbstractItemView* _view, QtPropertyModel* _model, QWidget* parent /* = 0 */)
    : QStyledItemDelegate(parent)
    , model(_model)
    , view(_view)
    , editorDataWasSet(false)
{
    DVASSERT(view);
    view->viewport()->installEventFilter(this);
}

QtPropertyItemDelegate::~QtPropertyItemDelegate()
{
}

void QtPropertyItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    if (index.column() == 1)
    {
        opt.textElideMode = Qt::ElideLeft;
        drawOptionalButtons(painter, opt, index);
    }

    auto* data = dynamic_cast<QtPropertyDataDavaVariant*>(model->itemFromIndex(index));
    if (
    (data != nullptr) &&
    (data->GetVariantValue().GetType() == DAVA::VariantType::TYPE_STRING) &&
    (data->GetVariantValue().AsString().find('\n') != DAVA::String::npos))
    {
        opt.text = opt.text.simplified();
    }

    view->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, view);
}

QSize QtPropertyItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto s = QStyledItemDelegate::sizeHint(option, index);
    static const int baseText = 17;
    static const int extra = 5;

    auto* data = dynamic_cast<QtPropertyDataDavaVariant*>(model->itemFromIndex(index));
    if (data != nullptr)
    {
        if (data->GetVariantValue().GetType() == DAVA::VariantType::TYPE_STRING)
        {
            const auto& text = data->GetValue().toString();
            if (!text.isEmpty() && text.contains('\n'))
            {
                s.setHeight(baseText);
            }
        }
    }

    s.setHeight(s.height() + extra);

    return s;
}

QWidget* QtPropertyItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QWidget* editWidget = nullptr;

    if (model == index.model())
    {
        QtPropertyData* data = model->itemFromIndex(index);

        if (nullptr != data)
        {
            editWidget = data->CreateEditor(parent, option);
        }

        // if widget wasn't created and it isn't checkable
        // let base class create editor
        if (nullptr == editWidget && !data->IsCheckable())
        {
            editWidget = QStyledItemDelegate::createEditor(parent, option, index);
        }
    }

    activeEditor = editWidget;
    editorDataWasSet = false;

    return editWidget;
}

void QtPropertyItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (editorDataWasSet)
    {
        return;
    }

    bool doneByInternalEditor = false;

    if (model == index.model())
    {
        QtPropertyData* data = model->itemFromIndex(index);
        if (nullptr != data)
        {
            doneByInternalEditor = data->SetEditorData(editor);
        }
    }

    if (!doneByInternalEditor)
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }

    editorDataWasSet = true;
}

bool QtPropertyItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* _model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseMove)
    {
        QtPropertyData* data = model->itemFromIndex(index);
        showButtons(data);
    }

    QStyleOptionViewItem opt(option);
    int delegatePadding = 0;
    QtPropertyData* data = model->itemFromIndex(index);
    if (index.column() == 1 && nullptr != data && data->GetButtonsCount() > 0)
    {
        delegatePadding += buttonSpacing;
        for (int i = data->GetButtonsCount() - 1; i >= 0; --i)
        {
            delegatePadding += data->GetButton(i)->width() + buttonSpacing;
        }
    }
    opt.rect.translate(delegatePadding, 0);

    return QStyledItemDelegate::editorEvent(event, model, opt, index);
}

bool QtPropertyItemDelegate::eventFilter(QObject* obj, QEvent* event)
{
    const bool needOverride = activeEditor && activeEditor->isVisible();

    if (needOverride)
    {
        switch (event->type())
        {
        case QEvent::MouseMove:
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            QModelIndex index = view->indexAt(me->pos());
            QtPropertyData* data = model->itemFromIndex(index);
            showButtons(data);
        }
        break;
        default:
            break;
        }
    }

    return QStyledItemDelegate::eventFilter(obj, event);
}

void QtPropertyItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* _model, const QModelIndex& index) const
{
    bool doneByInternalEditor = false;

    if (model == _model)
    {
        QtPropertyData* data = model->itemFromIndex(index);
        if (nullptr != data)
        {
            doneByInternalEditor = data->EditorDone(editor);
        }
    }

    if (!doneByInternalEditor)
    {
        QStyledItemDelegate::setModelData(editor, _model, index);
    }
}

void QtPropertyItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);

    // tune widget border and geometry
    if (nullptr != editor)
    {
        editor->setObjectName("customPropertyEditor");
        editor->setStyleSheet("#customPropertyEditor{ border: 1px solid gray; }");
        QRect r = option.rect;

        QtPropertyData* data = model->itemFromIndex(index);
        if (data)
        {
            const int n = data->GetButtonsCount();
            int padding = 0;
            for (int i = 0; i < n; i++)
            {
                QtPropertyToolButton* btn = data->GetButton(i);
                // Skip QComboBox button
                if (!btn->overlayed)
                {
                    padding += btn->geometry().width() + buttonSpacing;
                }
            }

            r.adjust(padding, 0, 0, 0);
        }

        editor->setGeometry(r);
    }
}

bool QtPropertyItemDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event == nullptr || view == nullptr)
        return false;

    bool showTooltip = false;

    if (nullptr != event && nullptr != view && event->type() == QEvent::ToolTip)
    {
        QRect rect = view->visualRect(index);
        QSize size = sizeHint(option, index);
        if (rect.width() < size.width())
        {
            showTooltip = true;
        }
    }

    if (!showTooltip && index.column() == 1)
    {
        QtPropertyData* data = model->itemFromIndex(index);
        if (data && data->GetToolTip().isValid())
        {
            showTooltip = true;
        }
    }

    if (showTooltip)
    {
        const QString toolTip = view->model()->data(index, Qt::ToolTipRole).toString();
        if (!toolTip.isEmpty())
        {
            const QRect updateRect(0, 0, 10, 10);
            QToolTip::showText(QCursor::pos(), toolTip, view, updateRect);
            return true;
        }
    }

    QToolTip::hideText();
    return false;
}

void QtPropertyItemDelegate::DrawButton(QPainter* painter, QStyleOptionViewItem& opt, QtPropertyToolButton* btn) const
{
    if (btn->height() != opt.rect.height())
    {
        QRect geom = btn->geometry();
        geom.setHeight(opt.rect.height());
        btn->setGeometry(geom);
    }

    int owYPos = opt.rect.y() + (opt.rect.height() - btn->height()) / 2;
    QPixmap pix = btn->grab();
    painter->drawPixmap(opt.rect.left(), owYPos, pix);
    btn->move(opt.rect.left(), owYPos);
    int padding = buttonSpacing + btn->width();
    opt.rect.adjust(padding, 0, 0, 0);
}

void QtPropertyItemDelegate::drawOptionalButtons(QPainter* painter, QStyleOptionViewItem& opt, const QModelIndex& index) const
{
    QtPropertyData* data = model->itemFromIndex(index);
    if (index.column() == 1 && nullptr != data && data->GetButtonsCount() > 0)
    {
        opt.rect.adjust(buttonSpacing, 0, 0, 0);

        // draw not overlaid widgets
        QtPropertyToolButton* addRemoveButton = nullptr;
        for (int i = data->GetButtonsCount() - 1; i >= 0 && nullptr == addRemoveButton; --i)
        {
            const auto& btn = data->GetButton(i);
            if (btn->objectName().contains("RemoveButton"))
            {
                addRemoveButton = btn;
                DrawButton(painter, opt, btn);
            }
        }

        for (int i = data->GetButtonsCount() - 1; i >= 0; --i)
        {
            QtPropertyToolButton* btn = data->GetButton(i);
            // update widget height
            if (addRemoveButton != btn)
            {
                DrawButton(painter, opt, btn);
            }
        }
    }
}

void QtPropertyItemDelegate::showButtons(QtPropertyData* data)
{
    if (data != lastHoverData)
    {
        hideButtons();
        showOptionalButtons(data);

        lastHoverData = data;
    }
}

void QtPropertyItemDelegate::showOptionalButtons(QtPropertyData* data)
{
    DVASSERT(visibleButtons.empty());
    if (nullptr != data)
    {
        int buttonCount = data->GetButtonsCount();
        visibleButtons.reserve(buttonCount);

        for (int i = 0; i < buttonCount; ++i)
        {
            QPointer<QtPropertyToolButton> button = data->GetButton(i);
            visibleButtons.push_back(button);
            button->show();
        }
    }
}

void QtPropertyItemDelegate::hideButtons()
{
    for (size_t i = 0; i < visibleButtons.size(); ++i)
    {
        QPointer<QtPropertyToolButton>& button = visibleButtons[i];
        if (button != nullptr)
            button->hide();
    }

    visibleButtons.clear();
}

void QtPropertyItemDelegate::invalidateButtons()
{
    showButtons(nullptr);
}
