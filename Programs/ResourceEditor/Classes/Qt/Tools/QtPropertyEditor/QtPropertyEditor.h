#ifndef __QT_PROPERTY_VIEW_H__
#define __QT_PROPERTY_VIEW_H__

#include <QTreeView>
#include <QTimer>
#include "QtPropertyData.h"
#include "QtPropertyModel.h"

class QtPropertyItemDelegate;

class QtPropertyEditor : public QTreeView
{
    Q_OBJECT

public:
    enum Style
    {
        DEFAULT_STYLE = 0,
        HEADER_STYLE,

        USER_STYLE
    };

    QtPropertyEditor(QWidget* parent = 0);
    ~QtPropertyEditor();

    void AppendProperties(DAVA::Vector<std::unique_ptr<QtPropertyData>>&& properties, const QModelIndex& parent = QModelIndex());
    QModelIndex AppendProperty(std::unique_ptr<QtPropertyData>&& data, const QModelIndex& parent = QModelIndex());
    void MergeProperty(std::unique_ptr<QtPropertyData>&& data, const QModelIndex& parent = QModelIndex());
    QModelIndex InsertProperty(std::unique_ptr<QtPropertyData>&& data, int row, const QModelIndex& parent = QModelIndex());
    QModelIndex AppendHeader(const QString& text);
    QModelIndex InsertHeader(const QString& text, int row);

    QtPropertyData* GetProperty(const QModelIndex& index) const;
    QtPropertyData* GetRootProperty() const;

    void FinishTreeCreation();

    bool GetEditTracking() const;
    void SetEditTracking(bool enabled);

    void RemoveProperty(const QModelIndex& index);
    void RemoveProperty(QtPropertyData* data);
    void RemovePropertyAll();

    void SetUpdateTimeout(int ms);
    int GetUpdateTimeout();
    void ShowButtonsUnderCursor();

    virtual void ApplyStyle(QtPropertyData* data, int style);

public slots:
    void Update();

    void OnExpanded(const QModelIndex& index);
    void OnCollapsed(const QModelIndex& index);

signals:
    void PropertyEdited(const QModelIndex& index);

protected:
    QtPropertyModel* curModel;
    QtPropertyItemDelegate* curItemDelegate;

    int updateTimeout;
    QTimer updateTimer;
    bool doUpdateOnPaintEvent;

    void leaveEvent(QEvent* event) override;
    void drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected slots:
    virtual void OnItemClicked(const QModelIndex&);
    virtual void OnItemEdited(const QModelIndex&);
    virtual void OnUpdateTimeout();

    virtual void rowsAboutToBeInserted(const QModelIndex& parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
    virtual void rowsOp(const QModelIndex& parent, int start, int end);
};

#endif // __QT_PROPERTY_VIEW_H__
