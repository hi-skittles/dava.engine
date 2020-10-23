#include "Command/SetCollectionItemValueCommand.h"

#include "Base/Introspection.h"
#include "Base/IntrospectionCollection.h"

namespace DAVA
{
class SetCollectionItemValueCommand::CollectionIteratorHelper
{
public:
    CollectionIteratorHelper(const ObjectHandle& object, const InspColl* collection_, VariantType key)
        : collection(collection_)
    {
        DVASSERT(collection != nullptr);

        const MetaInfo* itemKeyType = collection->ItemKeyType();
        if (itemKeyType == nullptr)
        {
            key = VariantType::Convert(key, VariantType::TYPE_INT32);
            DVASSERT(key.GetType() == VariantType::TYPE_INT32);
            DAVA::int32 index = key.AsInt32();

            iterator = collection->Begin(collection->Pointer(object.GetObjectPointer()));
            while (iterator != nullptr && index > 0)
            {
                iterator = collection->Next(iterator);
                index--;
            }
        }
        else
        {
            DVASSERT(key.Meta() == itemKeyType);
            iterator = collection->Begin(collection->Pointer(object.GetObjectPointer()));
            while (iterator != nullptr)
            {
                if (VariantType::LoadData(collection->ItemKeyData(iterator), key.Meta()) == key)
                {
                    break;
                }
                iterator = collection->Next(iterator);
            }
        }
    }

    ~CollectionIteratorHelper()
    {
        collection->Finish(iterator);
    }

    InspColl::Iterator GetIterator() const
    {
        return iterator;
    }

private:
    const InspColl* collection;
    InspColl::Iterator iterator;
};

SetCollectionItemValueCommand::SetCollectionItemValueCommand(const ObjectHandle& object_, const InspColl* collection_,
                                                             const VariantType& key_, const VariantType& newValue_)
    : Command()
    , object(object_)
    , collection(collection_)
    , key(key_)
    , newValue(newValue_)
{
    DVASSERT(object.IsValid());
    DVASSERT(collection != nullptr);
    DVASSERT(object.GetIntrospection()->Member(collection->Name()) != nullptr);
    DVASSERT(CollectionIteratorHelper(object, collection, key).GetIterator() != nullptr);

    const DAVA::MetaInfo* itemType = collection_->ItemType();
    if (newValue.Meta() != itemType)
    {
        newValue = VariantType::Convert(newValue, itemType);
        DVASSERT(newValue.Meta() == itemType);
    }

    CollectionIteratorHelper iterHelper(object, collection, key);
    oldValue = VariantType::LoadData(collection->ItemData(iterHelper.GetIterator()), collection->ItemType());
}

void SetCollectionItemValueCommand::Redo()
{
    SetValue(CollectionIteratorHelper(object, collection, key), newValue);
}

void SetCollectionItemValueCommand::Undo()
{
    SetValue(CollectionIteratorHelper(object, collection, key), oldValue);
}

void SetCollectionItemValueCommand::SetValue(const CollectionIteratorHelper& iterHelper, VariantType value)
{
    collection->ItemValueSet(iterHelper.GetIterator(), value.MetaObject());
}
}
