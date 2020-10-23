#ifndef __MATERIALS_TEMPLATE_FILTER_MODEL_H__
#define __MATERIALS_TEMPLATE_FILTER_MODEL_H__

#include <QSortFilterProxyModel>

class MaterialTemplateModel
: public QSortFilterProxyModel
{
    Q_OBJECT

public:
    MaterialTemplateModel(QObject* parent = NULL);
    ~MaterialTemplateModel();

private:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
};


#endif // __MATERIALS_TEMPLATE_FILTER_MODEL_H__
