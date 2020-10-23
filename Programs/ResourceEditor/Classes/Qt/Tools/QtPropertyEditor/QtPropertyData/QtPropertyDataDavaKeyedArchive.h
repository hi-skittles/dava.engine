#ifndef __QT_PROPERTY_DATA_DAVA_KEYEDARCHIVE_H__
#define __QT_PROPERTY_DATA_DAVA_KEYEDARCHIVE_H__

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData.h"
#include <REPlatform/Commands/KeyedArchiveCommand.h>
#include <REPlatform/Commands/RECommand.h>

#include <TArc/Utils/QtConnections.h>

#include <Base/Introspection.h>
#include <FileSystem/KeyedArchive.h>

#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class QtPropertyDataDavaKeyedArcive : public QtPropertyData
{
public:
    QtPropertyDataDavaKeyedArcive(const DAVA::FastName& name, DAVA::KeyedArchive* archive);
    ~QtPropertyDataDavaKeyedArcive() override;

    const DAVA::MetaInfo* MetaInfo() const override;
    std::unique_ptr<DAVA::Command> CreateLastCommand() const override;

    void FinishTreeCreation() override;

    DAVA::KeyedArchive* archive;

protected:
    mutable DAVA::RECommand* lastCommand;
    int lastAddedType;

    DAVA::QtConnections connections;

    QVariant GetValueInternal() const override;
    bool UpdateValueInternal() override;

private:
    void ChildCreate(const DAVA::FastName& key, DAVA::VariantType* value);

private:
    void AddKeyedArchiveField(QToolButton* button);
    void RemKeyedArchiveField(QToolButton* button);
    void NewKeyedArchiveFieldReady(const DAVA::String& key, const DAVA::VariantType& value);
    void RemKeyedArchiveField(const DAVA::FastName& key);
};

class KeyedArchiveItemWidget : public QWidget
{
    Q_OBJECT;

public:
    KeyedArchiveItemWidget(DAVA::KeyedArchive* arch, int defaultType = DAVA::VariantType::TYPE_STRING, QWidget* parent = NULL);
    ~KeyedArchiveItemWidget();

signals:
    void ValueReady(const DAVA::String& key, const DAVA::VariantType& value);

protected:
    DAVA::KeyedArchive* arch;

    QLineEdit* keyWidget;
    QComboBox* valueWidget;
    QComboBox* presetWidget;
    QPushButton* defaultBtn;

    virtual void showEvent(QShowEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

protected slots:
    void OkKeyPressed();
    void PreSetSelected(int index);
};

#endif // __QT_PROPERTY_DATA_DAVA_KEYEDARCHIVE_H__
