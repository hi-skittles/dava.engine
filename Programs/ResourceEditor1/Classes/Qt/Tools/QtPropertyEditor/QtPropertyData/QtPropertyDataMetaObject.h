#ifndef __QT_PROPERTY_DATA_META_OBJECT_H__
#define __QT_PROPERTY_DATA_META_OBJECT_H__

#include "Base/Introspection.h"
#include "../QtPropertyData.h"
#include "QtPropertyDataDavaVariant.h"
#include "Commands2/MetaObjModifyCommand.h"

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
    MetaObjModifyCommand* lastCommand;

    void SetValueInternal(const QVariant& value) override;
    bool UpdateValueInternal() override;
    bool EditorDoneInternal(QWidget* editor) override;
};

#endif // __QT_PROPERTY_DATA_META_OBJECT_H__
