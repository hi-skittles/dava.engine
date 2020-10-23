#pragma once

#include <QSortFilterProxyModel>

class BranchesFilterModel
: public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit BranchesFilterModel(QObject* parent = nullptr);

private:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
};
