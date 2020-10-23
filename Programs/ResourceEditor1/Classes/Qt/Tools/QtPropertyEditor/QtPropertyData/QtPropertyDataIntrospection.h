#ifndef __QT_PROPERTY_DATA_INTROSPECTION_H__
#define __QT_PROPERTY_DATA_INTROSPECTION_H__

#include "Base/Introspection.h"
#include "../QtPropertyData.h"

#include <QMap>

class QtPropertyDataDavaVariant;

class QtPropertyDataIntrospection : public QtPropertyData
{
public:
    QtPropertyDataIntrospection(const DAVA::FastName& name, void* object, const DAVA::InspInfo* info, bool autoAddChilds = true);
    virtual ~QtPropertyDataIntrospection();

    virtual const DAVA::MetaInfo* MetaInfo() const;
    static QtPropertyData* CreateMemberData(const DAVA::FastName& name, void* _object, const DAVA::InspMember* member);

    void* object;
    const DAVA::InspInfo* info;

protected:
    QMap<QtPropertyDataDavaVariant*, const DAVA::InspMember*> childVariantMembers;

    void AddMember(const DAVA::InspMember* member);
    virtual QVariant GetValueInternal() const;
};

#endif // __QT_PROPERTY_DATA_INTROSPECTION_H__
