#include "UnitTests/UnitTests.h"

#include "Command/ObjectHandle.h"
#include "Command/SetPropertyValueCommand.h"
#include "Command/SetCollectionItemValueCommand.h"

#include "Base/Introspection.h"
#include "Base/IntrospectionBase.h"

namespace CommandTestLocal
{
class TestClass : public DAVA::InspBase
{
public:
    DAVA::int32 intValue;
    DAVA::Vector<DAVA::uint32> collection;

    INTROSPECTION(TestClass,
                  MEMBER(intValue, "intValue", DAVA::I_EDIT | DAVA::I_VIEW)
                  COLLECTION(collection, "collection", DAVA::I_EDIT | DAVA::I_VIEW)
                  );
};
}

DAVA_TESTCLASS (CommandTest)
{
    DAVA_TEST (ObjectHandleCreationTest)
    {
        CommandTestLocal::TestClass testObject;
        {
            DAVA::ObjectHandle handle(&testObject, DAVA::MetaInfo::Instance<CommandTestLocal::TestClass>());
            TEST_VERIFY(handle.GetObjectPointer() == &testObject);
            TEST_VERIFY(handle.GetIntrospection() == testObject.GetTypeInfo());
        }

        {
            DAVA::ObjectHandle handle(static_cast<DAVA::InspBase*>(&testObject));
            TEST_VERIFY(handle.GetObjectPointer() == &testObject);
            TEST_VERIFY(handle.GetObjectType() == DAVA::MetaInfo::Instance<CommandTestLocal::TestClass>());
            TEST_VERIFY(handle.GetIntrospection() == testObject.GetTypeInfo());
        }
    }

    DAVA_TEST (SetPropertyValueCommandTest)
    {
        CommandTestLocal::TestClass testObject;
        testObject.intValue = 10;

        DAVA::ObjectHandle handle(&testObject);
        const DAVA::InspMember* inspMember = handle.GetIntrospection()->Member(DAVA::FastName("intValue"));
        {
            DAVA::SetPropertyValueCommand command(handle, inspMember, DAVA::VariantType(static_cast<DAVA::int32>(15)));
            command.Redo();
            TEST_VERIFY(testObject.intValue == 15);
            command.Undo();
            TEST_VERIFY(testObject.intValue == 10);
            command.Redo();
            TEST_VERIFY(testObject.intValue == 15);
        }

        {
            DAVA::SetPropertyValueCommand command(handle, inspMember, DAVA::VariantType(static_cast<DAVA::uint32>(20)));
            command.Redo();
            TEST_VERIFY(testObject.intValue == 20);
            command.Undo();
            TEST_VERIFY(testObject.intValue == 15);
            command.Redo();
            TEST_VERIFY(testObject.intValue == 20);
        }
    }

    DAVA_TEST (SetCollectionItemValueCommandTest)
    {
        CommandTestLocal::TestClass testObject;
        testObject.collection.push_back(10);
        testObject.collection.push_back(11);
        testObject.collection.push_back(12);
        DAVA::Vector<DAVA::uint32>& collection = testObject.collection;

        DAVA::ObjectHandle handle(&testObject);
        const DAVA::InspMember* inspMember = handle.GetIntrospection()->Member(DAVA::FastName("collection"));
        DAVA::SetCollectionItemValueCommand command0(handle, inspMember->Collection(),
                                                     DAVA::VariantType(static_cast<DAVA::int32>(0)),
                                                     DAVA::VariantType(static_cast<DAVA::int32>(15)));
        DAVA::SetCollectionItemValueCommand command1(handle, inspMember->Collection(),
                                                     DAVA::VariantType(static_cast<DAVA::uint32>(1)),
                                                     DAVA::VariantType(static_cast<DAVA::uint32>(20)));
        DAVA::SetCollectionItemValueCommand command2(handle, inspMember->Collection(),
                                                     DAVA::VariantType(static_cast<DAVA::int32>(2)),
                                                     DAVA::VariantType(static_cast<DAVA::uint32>(42)));

        TEST_VERIFY(collection[0] == 10);
        TEST_VERIFY(collection[1] == 11);
        TEST_VERIFY(collection[2] == 12);
        command0.Redo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 11);
        TEST_VERIFY(collection[2] == 12);
        command1.Redo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 20);
        TEST_VERIFY(collection[2] == 12);
        command2.Redo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 20);
        TEST_VERIFY(collection[2] == 42);

        command2.Undo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 20);
        TEST_VERIFY(collection[2] == 12);
        command1.Undo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 11);
        TEST_VERIFY(collection[2] == 12);
        command0.Undo();
        TEST_VERIFY(collection[0] == 10);
        TEST_VERIFY(collection[1] == 11);
        TEST_VERIFY(collection[2] == 12);

        command0.Redo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 11);
        TEST_VERIFY(collection[2] == 12);
        command1.Redo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 20);
        TEST_VERIFY(collection[2] == 12);
        command2.Redo();
        TEST_VERIFY(collection[0] == 15);
        TEST_VERIFY(collection[1] == 20);
        TEST_VERIFY(collection[2] == 42);
    }
};