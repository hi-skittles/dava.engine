#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "DataStorage/DataStorage.h"

using namespace DAVA;

DAVA_TESTCLASS (DataVaultTest)
{
    DAVA_TEST (TestFunction)
    {
        ScopedPtr<IDataStorage> storage(DataStorage::Create());
        storage->Push();
        storage->Clear();
        storage->Push();
        storage->SetStringValue("TestStringKey", "Test");
        storage->Push();

#if !defined(__DAVAENGINE_STEAM__) && (defined(__DAVAENGINE_WINDOWS__) || defined(__DAVAENGINE_LINUX__))
        return;
#endif

        String ret = storage->GetStringValue("TestStringKey");
        TEST_VERIFY("Test" == ret);
        storage->RemoveEntry("TestStringKey");
        storage->Push();
        ret = storage->GetStringValue("TestStringKey");
        TEST_VERIFY("Test" != ret);

        int64 iret = storage->GetLongValue("TestIntKey");
        TEST_VERIFY(0 == iret);

        storage->SetLongValue("TestIntKey", 1);
        storage->Push();
        iret = storage->GetLongValue("TestIntKey");
        TEST_VERIFY(1 == iret);

        storage->Clear();
        iret = storage->GetLongValue("TestIntKey");
        TEST_VERIFY(0 == iret);
    }
};
