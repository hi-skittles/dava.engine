#pragma once

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataDavaVariant.h"

#include <REPlatform/Commands/MetaObjModifyCommand.h>

#include <Base/Introspection.h>

class QtPropertyDataMetaObject : public QtPropertyDataDavaVariant
{
public:
    QtPropertyDataMetaObject(const DAVA::FastName& name, void* _object, const DAVA::MetaInfo* _meta);
    virtual ~QtPropertyDataMetaObject();

    const DAVA::MetaInfo* MetaInfo() const override;
    std::unique_ptr<DAVA::Command> CreateLastCommand() const override;

    void* object;
    const DAVA::MetaInfo* meta;

protected:
    DAVA::MetaObjModifyCommand* lastCommand;

    void SetValueInternal(const QVariant& value) override;
    bool UpdateValueInternal() override;
    bool EditorDoneInternal(QWidget* editor) override;
};
