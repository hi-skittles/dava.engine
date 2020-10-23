#pragma once

#include <Base/BaseTypes.h>
#include <QFrame>

class FindFilter;
class FindFilterWidget;

namespace Ui
{
class CompositeFindFilterWidget;
}

class CompositeFindFilterWidget : public QFrame
{
    Q_OBJECT
public:
    CompositeFindFilterWidget(QWidget* parent = nullptr);
    ~CompositeFindFilterWidget();

    std::unique_ptr<FindFilter> BuildFindFilter() const;

    void Reset();

signals:
    void FiltersChanged();

private slots:
    void AddFilterClicked();
    void RemoveFilterClicked();

private:
    void RemoveFilterWidget(FindFilterWidget* widget);

    DAVA::Set<FindFilterWidget*> filterWidgets;

    std::unique_ptr<Ui::CompositeFindFilterWidget> ui;
};
