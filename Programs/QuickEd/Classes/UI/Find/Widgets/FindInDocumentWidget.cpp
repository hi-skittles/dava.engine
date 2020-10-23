#include "UI/Find/Widgets/FindInDocumentWidget.h"
#include "UI/Find/Filters/FindFilter.h"
#include "UI/Find/Widgets/CompositeFindFilterWidget.h"

#include <TArc/Utils/Utils.h>
#include <QKeyEvent>

using namespace DAVA;

FindInDocumentWidget::FindInDocumentWidget(QWidget* parent)
    : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    findFiltersWidget = new CompositeFindFilterWidget(this);

    findNextButton = new QToolButton(this);
    findNextButton->setArrowType(Qt::RightArrow);
    findPreviousButton = new QToolButton(this);
    findPreviousButton->setArrowType(Qt::LeftArrow);
    findAllButton = new QToolButton(this);
    stopFindButton = new QToolButton(this);

    resultIndexCountLabel = new QLabel(this);

    connect(findFiltersWidget, SIGNAL(FiltersChanged()), this, SLOT(OnFiltersChanged()));

    layout->addWidget(findFiltersWidget);

    layout->addWidget(resultIndexCountLabel);
    layout->addSpacing(10);
    layout->addWidget(findPreviousButton);
    layout->addWidget(findNextButton);
    layout->addSpacing(10);
    layout->addWidget(findAllButton);
    layout->addSpacing(10);
    layout->addWidget(stopFindButton);

    setFocusProxy(findFiltersWidget);

    QAction* stopFindAction = new QAction(tr("Stop Find"), this);
    stopFindAction->setShortcut(Qt::Key_Escape);
    stopFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    stopFindAction->setIcon(DAVA::SharedIcon(":/QtTools/Icons/close-16.png"));
    connect(stopFindAction, SIGNAL(triggered()), this, SIGNAL(OnStopFind()));
    stopFindButton->setDefaultAction(stopFindAction);

    QAction* findNextAction = new QAction(tr("Find Next"), this);
    findNextAction->setShortcuts({ Qt::Key_Return, Qt::Key_Enter });
    findNextAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(findNextAction, SIGNAL(triggered()), this, SLOT(OnFindNextClicked()));
    findNextButton->setDefaultAction(findNextAction);

    QAction* findPreviousAction = new QAction(tr("Find Previous"), this);
    findPreviousAction->setShortcuts({ Qt::SHIFT + Qt::Key_Return, Qt::SHIFT + Qt::Key_Enter });
    findPreviousAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(findPreviousAction, SIGNAL(triggered()), this, SLOT(OnFindPreviousClicked()));
    findPreviousButton->setDefaultAction(findPreviousAction);

    QAction* findAllAction = new QAction(tr("Find All"), this);
    findAllAction->setShortcuts({ Qt::CTRL + Qt::Key_Return, Qt::CTRL + Qt::Key_Enter });
    findAllAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(findAllAction, SIGNAL(triggered()), this, SLOT(OnFindAllClicked()));
    findAllButton->setDefaultAction(findAllAction);

    addAction(stopFindAction);
    addAction(findNextAction);
    addAction(findPreviousAction);
    addAction(findAllAction);
}

std::shared_ptr<FindFilter> FindInDocumentWidget::BuildFindFilter() const
{
    return findFiltersWidget->BuildFindFilter();
}

void FindInDocumentWidget::Reset()
{
    findFiltersWidget->Reset();
}

void FindInDocumentWidget::SetResultIndex(DAVA::int32 _currentResultIndex)
{
    currentResultIndex = _currentResultIndex;
    UpdateResultIndexAndCountLabel();
}

void FindInDocumentWidget::SetResultCount(DAVA::int32 _currentResultCount)
{
    currentResultCount = _currentResultCount;
    UpdateResultIndexAndCountLabel();
}

void FindInDocumentWidget::OnFindNextClicked()
{
    emit OnFindNext();
}

void FindInDocumentWidget::OnFindPreviousClicked()
{
    emit OnFindPrevious();
}

void FindInDocumentWidget::OnFindAllClicked()
{
    emit OnFindAll();
}

void FindInDocumentWidget::OnFiltersChanged()
{
    std::shared_ptr<FindFilter> filter = findFiltersWidget->BuildFindFilter();

    emit OnFindFilterReady(filter);
}

void FindInDocumentWidget::UpdateResultIndexAndCountLabel()
{
    if (currentResultIndex < 0)
    {
        resultIndexCountLabel->setText(QString("-- / %2").arg(currentResultCount));
    }
    else
    {
        resultIndexCountLabel->setText(QString("%1 / %2").arg(currentResultIndex + 1).arg(currentResultCount));
    }
}
