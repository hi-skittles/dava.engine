#include "QtPropertyDataInspColl.h"
#include "QtPropertyDataIntrospection.h"
#include "QtPropertyDataMetaObject.h"

QtPropertyDataInspColl::QtPropertyDataInspColl(const DAVA::FastName& name, void* _object, const DAVA::InspColl* _collection, bool autoAddChilds)
    : QtPropertyData(name)
    , object(_object)
    , collection(_collection)
{
    if (NULL != collection && collection->Size(object) > 0 && autoAddChilds)
    {
        int index = 0;
        DAVA::MetaInfo* valueType = collection->ItemType();
        DAVA::InspColl::Iterator i = collection->Begin(object);
        while (NULL != i)
        {
            DAVA::FastName childName(std::to_string(index));
            if (NULL != valueType->GetIntrospection())
            {
                void* itemObject = collection->ItemData(i);
                const DAVA::InspInfo* itemInfo = valueType->GetIntrospection(itemObject);

                if (NULL != itemInfo && NULL != itemObject)
                {
                    std::unique_ptr<QtPropertyData> childData(new QtPropertyDataIntrospection(childName, itemObject, itemInfo));
                    ChildAdd(std::move(childData));
                }
                else
                {
                    QString s;
                    std::unique_ptr<QtPropertyData> childData(new QtPropertyData(childName, s.sprintf("[%p] Pointer", itemObject)));
                    childData->SetEnabled(false);
                    ChildAdd(std::move(childData));
                }
            }
            else
            {
                if (!valueType->IsPointer())
                {
                    std::unique_ptr<QtPropertyData> childData(new QtPropertyDataMetaObject(childName, collection->ItemPointer(i), valueType));
                    ChildAdd(std::move(childData));
                }
                else
                {
                    DAVA::FastName localChildName = childName;
                    if (collection->ItemKeyType() == DAVA::MetaInfo::Instance<DAVA::FastName>())
                    {
                        localChildName = *reinterpret_cast<const DAVA::FastName*>(collection->ItemKeyData(i));
                    }

                    QString s;
                    std::unique_ptr<QtPropertyData> childData(new QtPropertyData(localChildName, s.sprintf("[%p] Pointer", collection->ItemData(i))));
                    childData->SetEnabled(false);
                    ChildAdd(std::move(childData));
                }
            }

            index++;
            i = collection->Next(i);
        }
    }

    SetEnabled(false);
}

QtPropertyDataInspColl::~QtPropertyDataInspColl()
{
}

const DAVA::MetaInfo* QtPropertyDataInspColl::MetaInfo() const
{
    if (NULL != collection)
    {
        return collection->Type();
    }

    return NULL;
}

QVariant QtPropertyDataInspColl::GetValueInternal() const
{
    return QString().sprintf("Collection, size %d", collection->Size(object));
}
