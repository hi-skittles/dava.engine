#ifndef __QT_PROPERTY_DATA_INSP_MEMBER_H__
#define __QT_PROPERTY_DATA_INSP_MEMBER_H__

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataDavaVariant.h"
#include <REPlatform/Commands/InspMemberModifyCommand.h>

#include <Base/Introspection.h>

class QtPropertyDataInspMember : public QtPropertyDataDavaVariant
{
public:
    QtPropertyDataInspMember(const DAVA::FastName& name, void* _object, const DAVA::InspMember* _member);
    virtual ~QtPropertyDataInspMember();

    const DAVA::MetaInfo* MetaInfo() const override;
    std::unique_ptr<DAVA::Command> CreateLastCommand() const override;

    void* object;
    const DAVA::InspMember* member;

protected:
    DAVA::InspMemberModifyCommand* lastCommand;

    void SetValueInternal(const QVariant& value) override;
    void SetTempValueInternal(const QVariant& value) override;
    bool UpdateValueInternal() override;
    bool EditorDoneInternal(QWidget* editor) override;
};

#endif // __QT_PROPERTY_DATA_INSP_MEMBER_H__
