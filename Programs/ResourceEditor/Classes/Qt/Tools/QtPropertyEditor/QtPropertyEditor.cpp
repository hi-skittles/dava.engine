#include <QMouseEvent>
#include <QHeaderView>
#include <QPainter>
#include <QApplication>
#include <QScrollBar>

#include "QtPropertyEditor.h"
#include "QtPropertyModel.h"
#include "QtPropertyItemDelegate.h"

QtPropertyEditor::QtPropertyEditor(QWidget* parent /* = 0 */)
    : QTreeView(parent)
    , updateTimeout(0)
    , doUpdateOnPaintEvent(false)
{
    curModel = new QtPropertyModel(viewport());
    setModel(curModel);

    curItemDelegate = new QtPropertyItemDelegate(this, curModel);
    setItemDelegate(curItemDelegate);

    QObject::connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(OnItemClicked(const QModelIndex&)));
    QObject::connect(this, SIGNAL(expanded(const QModelIndex&)), this, SLOT(OnExpanded(const QModelIndex&)));
    QObject::connect(this, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(OnCollapsed(const QModelIndex&)));
    QObject::connect(curModel, SIGNAL(PropertyEdited(const QModelIndex&)), this, SLOT(OnItemEdited(const QModelIndex&)));
    QObject::connect(curModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(rowsAboutToBeInserted(const QModelIndex&, int, int)));
    QObject::connect(curModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)), this, SLOT(rowsAboutToBeRemoved(const QModelIndex&, int, int)));
    QObject::connect(curModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(rowsOp(const QModelIndex&, int, int)));
    QObject::connect(curModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(rowsOp(const QModelIndex&, int, int)));
    QObject::connect(&updateTimer, SIGNAL(timeout()), this, SLOT(OnUpdateTimeout()));

    setMouseTracking(true);
}

QtPropertyEditor::~QtPropertyEditor()
{
}

void QtPropertyEditor::AppendProperties(DAVA::Vector<std::unique_ptr<QtPropertyData>>&& properties, const QModelIndex& parent /*= QModelIndex()*/)
{
    curModel->AppendProperties(std::move(properties), parent);
}

QModelIndex QtPropertyEditor::AppendProperty(std::unique_ptr<QtPropertyData>&& data, const QModelIndex& parent)
{
    return curModel->AppendProperty(std::move(data), parent);
}

void QtPropertyEditor::MergeProperty(std::unique_ptr<QtPropertyData>&& data, QModelIndex const& parent)
{
    curModel->MergeProperty(std::move(data), parent);
}

QModelIndex QtPropertyEditor::InsertProperty(std::unique_ptr<QtPropertyData>&& data, int row, const QModelIndex& parent)
{
    return curModel->InsertProperty(std::move(data), row, parent);
}

QModelIndex QtPropertyEditor::AppendHeader(const QString& text)
{
    QtPropertyData* propHeader = new QtPropertyData(DAVA::FastName(text.toStdString()));
    QModelIndex result = AppendProperty(std::unique_ptr<QtPropertyData>(propHeader));

    ApplyStyle(propHeader, HEADER_STYLE);
    return result;
}

QModelIndex QtPropertyEditor::InsertHeader(const QString& text, int row)
{
    QModelIndex propHeader = InsertProperty(std::unique_ptr<QtPropertyData>(new QtPropertyData(DAVA::FastName(text.toStdString()))), row);

    ApplyStyle(GetProperty(propHeader), HEADER_STYLE);
    return propHeader;
}

QtPropertyData* QtPropertyEditor::GetProperty(const QModelIndex& index) const
{
    return curModel->itemFromIndex(index);
}

QtPropertyData* QtPropertyEditor::GetRootProperty() const
{
    return curModel->rootItem();
}

void QtPropertyEditor::FinishTreeCreation()
{
    curModel->FinishTreeCreation();
}

void QtPropertyEditor::RemoveProperty(const QModelIndex& index)
{
    curModel->RemoveProperty(index);
}

void QtPropertyEditor::RemoveProperty(QtPropertyData* data)
{
    curModel->RemoveProperty(curModel->indexFromItem(data));
}

void QtPropertyEditor::RemovePropertyAll()
{
    curModel->RemovePropertyAll();
}

void QtPropertyEditor::SetEditTracking(bool enabled)
{
    curModel->SetEditTracking(enabled);
}

bool QtPropertyEditor::GetEditTracking() const
{
    return curModel->GetEditTracking();
}

void QtPropertyEditor::SetUpdateTimeout(int ms)
{
    updateTimeout = ms;

    if (0 != updateTimeout)
    {
        updateTimer.start(updateTimeout);
    }
    else
    {
        updateTimer.stop();
    }
}

int QtPropertyEditor::GetUpdateTimeout()
{
    return updateTimeout;
}

void QtPropertyEditor::Update()
{
    OnUpdateTimeout();
}

void QtPropertyEditor::OnUpdateTimeout()
{
    if (state() != QAbstractItemView::EditingState)
    {
        doUpdateOnPaintEvent = false;
        curModel->UpdateStructure();
    }

    SetUpdateTimeout(updateTimeout);
}

void QtPropertyEditor::OnExpanded(const QModelIndex& index)
{
    ShowButtonsUnderCursor();
}

void QtPropertyEditor::OnCollapsed(const QModelIndex& index)
{
    ShowButtonsUnderCursor();
}

void QtPropertyEditor::drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItemV4 opt = option;
    // We should set font family manually, to set familyResolved flag in font.
    // If we don't do this, Qt will get resolve family almost randomly
    opt.font.setFamily(opt.font.family());
    QColor gridColor = opt.palette.color(QPalette::Normal, QPalette::Window);

    // draw horizontal bottom line
    painter->setPen(gridColor);
    painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());

    // adjust rect, so that grid line wont be overdrawn
    opt.rect.adjust(0, 0, 0, -1);

    // draw row
    QTreeView::drawRow(painter, opt, index);

    // draw vertical line
    if (!(option.state & QStyle::State_Selected))
    {
        QHeaderView* hdr = header();
        if (NULL != hdr && hdr->count() > 1)
        {
            int sz = hdr->sectionSize(0);
            QScrollBar* scroll = horizontalScrollBar();
            if (scroll != NULL)
            {
                sz -= scroll->value();
            }

            QPoint p1 = option.rect.topLeft();
            QPoint p2 = option.rect.bottomLeft();

            p1.setX(p1.x() + sz - 1);
            p2.setX(p2.x() + sz - 1);

            painter->setPen(gridColor);
            painter->drawLine(p1, p2);
        }
    }
}

void QtPropertyEditor::ApplyStyle(QtPropertyData* data, int style)
{
    if (NULL != data)
    {
        switch (style)
        {
        case DEFAULT_STYLE:
            data->ResetStyle();
            break;

        case HEADER_STYLE:
        {
            QFont boldFont = data->GetFont();
            boldFont.setBold(true);
            data->SetFont(boldFont);
            data->SetBackground(palette().alternateBase());
            data->SetEditable(false);
        }
        break;

        default:
            break;
        }
    }
}

void QtPropertyEditor::leaveEvent(QEvent* event)
{
    curItemDelegate->invalidateButtons();
}

void QtPropertyEditor::OnItemClicked(const QModelIndex& index)
{
    edit(index, QAbstractItemView::DoubleClicked, NULL);
}

void QtPropertyEditor::OnItemEdited(const QModelIndex& index)
{
    emit PropertyEdited(index);
}

void QtPropertyEditor::rowsAboutToBeInserted(const QModelIndex& parent, int start, int end)
{
    curItemDelegate->invalidateButtons();
}

void QtPropertyEditor::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    curItemDelegate->invalidateButtons();
}

void QtPropertyEditor::rowsOp(const QModelIndex& parent, int start, int end)
{
    ShowButtonsUnderCursor();
}

void QtPropertyEditor::ShowButtonsUnderCursor()
{
    QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
    curItemDelegate->showButtons(GetProperty(indexAt(pos)));
}
