#ifndef __QT_PROPERTY_KEYED_ARCHIVE_MEMBER_H__
#define __QT_PROPERTY_KEYED_ARCHIVE_MEMBER_H__

#include "Base/Introspection.h"
#include "../QtPropertyData.h"
#include "QtPropertyDataDavaVariant.h"
#include "Commands2/KeyedArchiveCommand.h"

class QtPropertyKeyedArchiveMember : public QtPropertyDataDavaVariant
{
public:
    QtPropertyKeyedArchiveMember(const DAVA::FastName& name, DAVA::KeyedArchive* archive, const DAVA::String& key);
    virtual ~QtPropertyKeyedArchiveMember();

    DAVA::KeyedArchive* archive;
    DAVA::String key;

protected:
    KeyeadArchiveSetValueCommand* lastCommand;

    void SetValueInternal(const QVariant& value) override;
    bool UpdateValueInternal() override;
    bool EditorDoneInternal(QWidget* editor) override;

    std::unique_ptr<DAVA::Command> CreateLastCommand() const override;

private:
    void CheckAndFillPresetValues();
};

#endif // __QT_PROPERTY_KEYED_ARCHIVE_MEMBER_H__
