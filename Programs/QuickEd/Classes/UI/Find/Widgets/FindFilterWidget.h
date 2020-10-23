#pragma once

#include <Base/BaseTypes.h>
#include <QComboBox>
#include <QHBoxLayout>
#include <QWidget>
#include <QToolButton>

class FindFilter;
class FindFilterEditor;

class FindFilterWidget : public QWidget
{
    Q_OBJECT
public:
    FindFilterWidget(QWidget* parent = nullptr);

    std::shared_ptr<FindFilter> BuildFindFilter() const;

signals:
    void AddAnotherFilter();
    void RemoveFilter();
    void FilterChanged();

private slots:
    void FilterSelected(int index);

private:
    QHBoxLayout* layout = nullptr;
    QToolButton* addFilterButton = nullptr;
    QToolButton* removeFilterButton = nullptr;
    QToolButton* negationButton = nullptr;
    QComboBox* filterCombobox = nullptr;

    QHBoxLayout* innerLayout = nullptr;

    FindFilterEditor* editor = nullptr;
};
