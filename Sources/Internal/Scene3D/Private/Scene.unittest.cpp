#include "UnitTests/UnitTests.h"
#include "Scene3D/Scene.h"
#include "Entity/SceneSystem.h"
#include "Entity/SingletonComponent.h"

using namespace DAVA;

class Mysystem : public SceneSystem
{
public:
    Mysystem(Scene* scene)
        : SceneSystem(scene)
    {
    }

    void PrepareForRemove() override
    {
    }
};

class MyComponent : public SingletonComponent
{
};

DAVA_TESTCLASS (SceneTest)
{
    DAVA_TEST (GetSystem)
    {
        Scene* scene = new Scene();
        SCOPE_EXIT
        {
            SafeRelease(scene);
        };

        Mysystem* mySystem = new Mysystem(scene);
        SCOPE_EXIT
        {
            delete mySystem;
        };

        TEST_VERIFY(scene->GetSystem<Mysystem>() == nullptr);

        scene->AddSystem(mySystem, 0);
        TEST_VERIFY(scene->GetSystem<Mysystem>() == mySystem);

        scene->RemoveSystem(mySystem);
        TEST_VERIFY(scene->GetSystem<Mysystem>() == nullptr);
    }

    DAVA_TEST (SingletonComponent)
    {
        Scene* scene = new Scene();
        SCOPE_EXIT
        {
            SafeRelease(scene);
        };

        MyComponent* myComponent = new MyComponent();
        SCOPE_EXIT
        {
            delete myComponent;
        };

        TEST_VERIFY(scene->GetSingletonComponent<MyComponent>() == nullptr);

        scene->AddSingletonComponent(myComponent);
        TEST_VERIFY(scene->GetSingletonComponent<MyComponent>() == myComponent);

        scene->RemoveSingletonComponent(myComponent);
        TEST_VERIFY(scene->GetSingletonComponent<MyComponent>() == nullptr);
    }
};