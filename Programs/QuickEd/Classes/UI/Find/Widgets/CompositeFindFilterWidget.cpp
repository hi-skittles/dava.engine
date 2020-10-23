#include "UI/Find/Widgets/CompositeFindFilterWidget.h"
#include "UI/Find/Widgets/FindFilterWidget.h"
#include "UI/Find/Filters/CompositeFilter.h"
#include "ui_CompositeFindFilterWidget.h"

using namespace DAVA;

CompositeFindFilterWidget::CompositeFindFilterWidget(QWidget* parent)
    : QFrame(parent)
    , ui(new Ui::CompositeFindFilterWidget())
{
    ui->setupUi(this);

    ui->filtersList->setAlignment(Qt::AlignTop);
    Reset();
}

CompositeFindFilterWidget::~CompositeFindFilterWidget() = default;

void CompositeFindFilterWidget::Reset()
{
    Set<FindFilterWidget*>::iterator iter = filterWidgets.begin();
    while (iter != filterWidgets.end())
    {
        RemoveFilterWidget(*iter);

        iter = filterWidgets.begin();
    }

    AddFilterClicked();
}

void CompositeFindFilterWidget::AddFilterClicked()
{
    FindFilterWidget* filterWidget = new FindFilterWidget();
    ui->filtersList->addWidget(filterWidget);

    QObject::connect(filterWidget, SIGNAL(AddAnotherFilter()), this, SLOT(AddFilterClicked()));
    QObject::connect(filterWidget, SIGNAL(RemoveFilter()), this, SLOT(RemoveFilterClicked()));
    QObject::connect(filterWidget, SIGNAL(FilterChanged()), this, SIGNAL(FiltersChanged()));

    setFocusProxy(filterWidget);

    filterWidgets.insert(filterWidget);

    emit FiltersChanged();
}

void CompositeFindFilterWidget::RemoveFilterClicked()
{
    if (filterWidgets.size() > 1)
    {
        FindFilterWidget* widget = qobject_cast<FindFilterWidget*>(QObject::sender());

        RemoveFilterWidget(widget);

        emit FiltersChanged();
    }
}

void CompositeFindFilterWidget::RemoveFilterWidget(FindFilterWidget* widget)
{
    filterWidgets.erase(widget);
    ui->filtersList->removeWidget(widget);
    widget->deleteLater();
}

std::unique_ptr<FindFilter> CompositeFindFilterWidget::BuildFindFilter() const
{
    DAVA::Vector<std::shared_ptr<FindFilter>> filters;

    for (FindFilterWidget* const filterWidget : filterWidgets)
    {
        filters.push_back(filterWidget->BuildFindFilter());
    }

    return std::make_unique<CompositeFilter>(filters);
}
