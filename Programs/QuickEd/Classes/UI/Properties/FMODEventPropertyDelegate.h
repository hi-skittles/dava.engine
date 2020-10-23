#pragma once

#include "BasePropertyDelegate.h"
#include "Base/BaseTypes.h"
#include "Sound/SoundEvent.h"

class Project;

class FMODEventPropertyDelegate : public BasePropertyDelegate
{
    Q_OBJECT
public:
    explicit FMODEventPropertyDelegate(PropertiesTreeItemDelegate* delegate);
    ~FMODEventPropertyDelegate();

    QWidget* createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    bool setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions) override;

private slots:
    void playEventClicked();
    void selectEventClicked();
    void clearEventClicked();
    void OnEditingFinished();
    void OnTextChanged(const QString& text);

private:
    bool IsSoundEventValid(const QString& eventName);
    void StopCurrentPlayingEvent();

    QPointer<QLineEdit> lineEdit = nullptr;
    DAVA::Vector<DAVA::String> eventNames;

    DAVA::RefPtr<DAVA::SoundEvent> currentPlayingEvent;
};
