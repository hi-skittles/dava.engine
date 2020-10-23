#ifndef __QT_PROPERTY_DATA_INTRO_COLLECTION_H__
#define __QT_PROPERTY_DATA_INTRO_COLLECTION_H__

#include "Base/Introspection.h"
#include "../QtPropertyData.h"

class QtPropertyDataInspColl : public QtPropertyData
{
public:
    QtPropertyDataInspColl(const DAVA::FastName& name, void* _object, const DAVA::InspColl* _collection, bool autoAddChilds = true);
    virtual ~QtPropertyDataInspColl();

    virtual const DAVA::MetaInfo* MetaInfo() const;

    void* object;
    const DAVA::InspColl* collection;

protected:
    virtual QVariant GetValueInternal() const;
};

#endif // __QT_PROPERTY_DATA_INTRO_COLLECTION_H__
