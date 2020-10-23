#pragma once

#include <Base/BaseTypes.h>
#include <QAction>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QWidget>

class FindFilter;
class CompositeFindFilterWidget;

class FindInDocumentWidget : public QWidget
{
    Q_OBJECT
public:
    FindInDocumentWidget(QWidget* parent = nullptr);

    std::shared_ptr<FindFilter> BuildFindFilter() const;

    void Reset();

    void SetResultIndex(DAVA::int32 currentResultIndex);
    void SetResultCount(DAVA::int32 currentResultCount);

signals:
    void OnFindFilterReady(std::shared_ptr<FindFilter> filter);
    void OnFindNext();
    void OnFindPrevious();
    void OnFindAll();
    void OnStopFind();

private slots:
    void OnFindNextClicked();
    void OnFindPreviousClicked();
    void OnFindAllClicked();
    void OnFiltersChanged();

private:
    void UpdateResultIndexAndCountLabel();

    QHBoxLayout* layout = nullptr;
    CompositeFindFilterWidget* findFiltersWidget = nullptr;
    QToolButton* findNextButton = nullptr;
    QToolButton* findPreviousButton = nullptr;
    QToolButton* findAllButton = nullptr;
    QToolButton* stopFindButton = nullptr;
    QLabel* resultIndexCountLabel = nullptr;

    DAVA::int32 currentResultIndex = -1;
    DAVA::int32 currentResultCount = 0;
};
