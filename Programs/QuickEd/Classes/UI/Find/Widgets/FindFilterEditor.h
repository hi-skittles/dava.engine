#pragma once

#include <Base/BaseTypes.h>
#include <QWidget>
#include <memory>

class FindFilter;

class FindFilterEditor
: public QWidget
{
    Q_OBJECT
public:
    FindFilterEditor(QWidget* parent);

    virtual std::unique_ptr<FindFilter> BuildFindFilter() = 0;

signals:
    void FilterChanged();
};
