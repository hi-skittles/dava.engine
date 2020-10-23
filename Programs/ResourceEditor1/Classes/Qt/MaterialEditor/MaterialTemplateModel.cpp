#include "MaterialTemplateModel.h"

MaterialTemplateModel::MaterialTemplateModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

MaterialTemplateModel::~MaterialTemplateModel()
{
}

bool MaterialTemplateModel::filterAcceptsRow(int source_row, QModelIndex const& source_parent) const
{
    return true;
}
