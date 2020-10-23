#ifndef __DAVAFRAMEWORK_SETCOLLECTIONITEMVALUECOMMAND_H__
#define __DAVAFRAMEWORK_SETCOLLECTIONITEMVALUECOMMAND_H__

#include "Command/Command.h"
#include "Command/ObjectHandle.h"

#include "FileSystem/VariantType.h"

namespace DAVA
{
class InspColl;
class SetCollectionItemValueCommand : public Command
{
public:
    SetCollectionItemValueCommand(const ObjectHandle& object, const InspColl* collection,
                                  const VariantType& key, const VariantType& newValue);

    void Redo() override;
    void Undo() override;

private:
    class CollectionIteratorHelper;
    void SetValue(const CollectionIteratorHelper& iterHelper, VariantType value);

private:
    ObjectHandle object;
    const InspColl* collection;
    VariantType key;
    VariantType oldValue;
    VariantType newValue;
};
}

#endif // __DAVAFRAMEWORK_SETCOLLECTIONITEMVALUECOMMAND_H__