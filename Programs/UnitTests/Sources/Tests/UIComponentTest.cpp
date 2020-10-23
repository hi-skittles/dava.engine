#include "UnitTests/UnitTests.h"

#include <Engine/Engine.h>
#include <UI/Components/UIComponent.h>
#include <Entity/ComponentManager.h>

using namespace DAVA;

class TestUIComponent : public UIComponent
{
};

DAVA_TESTCLASS (UIComponentTest)
{
    ComponentManager* globalCM;
    //ComponentManager* localCM;

    UIComponentTest()
    {
        globalCM = GetEngineContext()->componentManager;
        //localCM = new ComponentManager();
    }

    ~UIComponentTest()
    {
        //SafeDelete(localCM);
    }

    DAVA_TEST (ComponentManagerTest)
    {
        TEST_VERIFY(globalCM);
        TEST_VERIFY(globalCM->GetUIComponentsCount() != 0);

        //localCM->RegisterComponent<TestUIComponent>();
        //TEST_VERIFY(localCM->GetComponentsCount() == 1);
    }
};
