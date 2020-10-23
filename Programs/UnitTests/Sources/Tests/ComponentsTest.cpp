#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Scene3D/Components/ActionComponent.h"
#include "Scene3D/Components/AnimationComponent.h"
#include "Scene3D/Components/CameraComponent.h"
#include "Scene3D/Components/CustomPropertiesComponent.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/SwitchComponent.h"
#include "Scene3D/Components/TextComponent.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Entity/ComponentUtils.h"
#include "Reflection/ReflectionRegistrator.h"

using namespace DAVA;

class SingleComponentSystem : public SceneSystem
{
public:
    SingleComponentSystem(Scene* scene);

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;

    void AddComponent(Entity* entity, Component* component) override;
    void RemoveComponent(Entity* entity, Component* component) override;
    void PrepareForRemove() override;

    uint32 GetEnititesCount() const;
    uint32 GetComponentsCount() const;

    Vector<Component*> components;
    Vector<Entity*> entities;
};

class MultiComponentSystem : public SceneSystem
{
public:
    MultiComponentSystem(Scene* scene);

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;

    void AddComponent(Entity* entity, Component* component) override;
    void RemoveComponent(Entity* entity, Component* component) override;
    void PrepareForRemove() override;

    uint32 GetEnititesCount() const;

    template <typename T>
    uint32 GetComponentsCount() const;

    Map<const Type*, Vector<Component*>> components;
    Vector<Entity*> entities;
};

template <class T>
void RemovePointerFromVector(DAVA::Vector<T*>& elements, const T* element)
{
    size_t size = elements.size();
    for (size_t index = 0; index < size; ++index)
    {
        if (element == elements[index])
        {
            elements[index] = elements[size - 1];
            elements.pop_back();
            return;
        }
    }
    DVASSERT(0);
}

SingleComponentSystem::SingleComponentSystem(Scene* scene)
    : SceneSystem(scene)
{
}

void SingleComponentSystem::AddEntity(Entity* entity)
{
    entities.push_back(entity);

    ComponentManager* cm = GetEngineContext()->componentManager;
    const Vector<const Type*>& registeredComponentsTypes = cm->GetRegisteredSceneComponents();

    for (const Type* type : registeredComponentsTypes)
    {
        uint32 runtimeId = cm->GetRuntimeComponentId(type);

        if (GetRequiredComponents().test(runtimeId))
        {
            uint32 componentsCount = entity->GetComponentCount(type);
            for (uint32 c = 0; c < componentsCount; ++c)
            {
                AddComponent(entity, entity->GetComponent(type, c));
            }
        }
    }
}

void SingleComponentSystem::RemoveEntity(Entity* entity)
{
    ComponentManager* cm = GetEngineContext()->componentManager;
    const Vector<const Type*>& registeredComponentsTypes = cm->GetRegisteredSceneComponents();

    for (const Type* type : registeredComponentsTypes)
    {
        uint32 runtimeId = cm->GetRuntimeComponentId(type);

        if (GetRequiredComponents().test(runtimeId))
        {
            uint32 componentsCount = entity->GetComponentCount(type);

            for (uint32 c = 0; c < componentsCount; ++c)
            {
                RemoveComponent(entity, entity->GetComponent(type, c));
            }
        }
    }
    RemovePointerFromVector(entities, entity);
}

void SingleComponentSystem::AddComponent(Entity* entity, Component* component)
{
    components.push_back(component);
}

void SingleComponentSystem::RemoveComponent(Entity* entity, Component* component)
{
    RemovePointerFromVector(components, component);
}

void SingleComponentSystem::PrepareForRemove()
{
    entities.clear();
    components.clear();
}

uint32 SingleComponentSystem::GetEnititesCount() const
{
    return static_cast<uint32>(entities.size());
}

uint32 SingleComponentSystem::GetComponentsCount() const
{
    return static_cast<uint32>(components.size());
}

//=============================================
MultiComponentSystem::MultiComponentSystem(Scene* scene)
    : SceneSystem(scene)
{
}

void MultiComponentSystem::AddEntity(Entity* entity)
{
    entities.push_back(entity);

    ComponentManager* cm = GetEngineContext()->componentManager;
    const Vector<const Type*>& registeredComponentsTypes = cm->GetRegisteredSceneComponents();

    for (const Type* type : registeredComponentsTypes)
    {
        uint32 runtimeId = cm->GetRuntimeComponentId(type);

        if (GetRequiredComponents().test(runtimeId))
        {
            uint32 componentsCount = entity->GetComponentCount(type);
            for (uint32 c = 0; c < componentsCount; ++c)
            {
                AddComponent(entity, entity->GetComponent(type, c));
            }
        }
    }
}

void MultiComponentSystem::RemoveEntity(Entity* entity)
{
    ComponentManager* cm = GetEngineContext()->componentManager;
    const Vector<const Type*>& registeredComponentsTypes = cm->GetRegisteredSceneComponents();

    for (const Type* type : registeredComponentsTypes)
    {
        uint32 runtimeId = cm->GetRuntimeComponentId(type);

        if (GetRequiredComponents().test(runtimeId))
        {
            uint32 componentsCount = entity->GetComponentCount(type);

            for (uint32 c = 0; c < componentsCount; ++c)
            {
                RemoveComponent(entity, entity->GetComponent(type, c));
            }
        }
    }
    RemovePointerFromVector(entities, entity);
}

void MultiComponentSystem::AddComponent(Entity* entity, Component* component)
{
    components[component->GetType()].push_back(component);
}

void MultiComponentSystem::RemoveComponent(Entity* entity, Component* component)
{
    RemovePointerFromVector(components[component->GetType()], component);
}

void MultiComponentSystem::PrepareForRemove()
{
    entities.clear();
    components.clear();
}

uint32 MultiComponentSystem::GetEnititesCount() const
{
    return static_cast<uint32>(entities.size());
}

template <typename T>
uint32 MultiComponentSystem::GetComponentsCount() const
{
    auto found = components.find(Type::Instance<T>());
    if (found != components.end())
    {
        return static_cast<uint32>(found->second.size());
    }

    return 0;
}

template <typename T>
uint32 Index(T* t)
{
    ComponentManager* cm = GetEngineContext()->componentManager;
    return cm->GetRuntimeComponentId(Type::Instance<T>());
}

#define FAKE_COMPONENT(X) \
struct X : public Component { DAVA_VIRTUAL_REFLECTION(X, Component); Component* Clone(Entity* toEntity) override { return nullptr; } }; \
DAVA_VIRTUAL_REFLECTION_IMPL(X) { ReflectionRegistrator<X>::Begin().ConstructorByPointer().End(); } \

#define FAKE_COMPONENT_REG(X, NAME) \
DAVA_REFLECTION_REGISTER_CUSTOM_PERMANENT_NAME(X, NAME); \
GetEngineContext()->componentManager->RegisterComponent<X>()

FAKE_COMPONENT(Component1);
FAKE_COMPONENT(Component2);
FAKE_COMPONENT(Component3);
FAKE_COMPONENT(Component4);
FAKE_COMPONENT(Component5);
FAKE_COMPONENT(Component6);
FAKE_COMPONENT(Component7);
FAKE_COMPONENT(Component8);
FAKE_COMPONENT(Component9);
FAKE_COMPONENT(Component10);

DAVA_TESTCLASS (ComponentsTest)
{
    DAVA_TEST (SortedComponentIdTest)
    {
        FAKE_COMPONENT_REG(Component1, "a");
        FAKE_COMPONENT_REG(Component2, "c");
        FAKE_COMPONENT_REG(Component3, "b");
        FAKE_COMPONENT_REG(Component4, "z");
        FAKE_COMPONENT_REG(Component5, "f");
        FAKE_COMPONENT_REG(Component6, "e");
        FAKE_COMPONENT_REG(Component7, ".");
        FAKE_COMPONENT_REG(Component8, ",");
        FAKE_COMPONENT_REG(Component9, "`");
        FAKE_COMPONENT_REG(Component10, "'");

        ComponentManager* cm = GetEngineContext()->componentManager;

        Vector<const Type*> components = cm->GetRegisteredSceneComponents();

        std::sort(components.begin(), components.end(), [](const Type* l, const Type* r) {
            const ReflectedType* lRefType = ReflectedTypeDB::GetByType(l);
            const ReflectedType* rRefType = ReflectedTypeDB::GetByType(r);

            const String& lName = lRefType->GetPermanentName();
            const String& rName = rRefType->GetPermanentName();

            return lName < rName;
        });

        for (size_t i = 0; i < components.size(); ++i)
        {
            uint32 sortedId = cm->GetSortedComponentId(components[i]);
            TEST_VERIFY(sortedId == i);
        }
    }

    DAVA_TEST (EntityComponentsGettersTest)
    {
        Entity* entity = new Entity();

        TEST_VERIFY(entity->GetComponentCount() == 1); // TransformComponent is added in Entity constructor
        TEST_VERIFY(entity->GetComponent<TransformComponent>() != nullptr);

        LightComponent* c;

        const uint32 COMP_MAX = 50;

        for (uint32 i = 0; i < COMP_MAX; ++i)
        {
            c = new LightComponent();
            entity->AddComponent(c);

            TEST_VERIFY(entity->GetComponentCount<LightComponent>() == i + 1);
            TEST_VERIFY(entity->GetComponentCount(Type::Instance<LightComponent>()) == i + 1);

            Component* c1 = entity->GetComponent<LightComponent>(i);
            Component* c2 = entity->GetComponent(Type::Instance<LightComponent>(), i);

            TEST_VERIFY(c1 == c2);
        }

        TEST_VERIFY(entity->GetComponentCount() == COMP_MAX + 1);

        TEST_VERIFY(entity->GetComponent<LightComponent>(COMP_MAX - 1) == c);

        SafeRelease(entity); // Components will be released in Entity destructor
    }

    DAVA_TEST (EntityComponentsMaskAndTypeTest)
    {
        Entity* entity = new Entity();

        ComponentMask mask = ComponentUtils::MakeMask<TransformComponent>();

        TEST_VERIFY((entity->GetAvailableComponentMask() & mask) == mask); // TransformComponent is added in Entity constructor

        TransformComponent* c = entity->GetComponent<TransformComponent>();

        TEST_VERIFY(entity->GetAvailableComponentMask().test(Index(c)));

        entity->RemoveComponent<TransformComponent>();

        TEST_VERIFY(entity->GetAvailableComponentMask().none());

        entity->AddComponent(new TransformComponent());
        entity->AddComponent(new LightComponent());
        entity->AddComponent(new ActionComponent());
        entity->AddComponent(new AnimationComponent());
        entity->AddComponent(new CameraComponent());
        entity->AddComponent(new CustomPropertiesComponent());
        entity->AddComponent(new ParticleEffectComponent());
        entity->AddComponent(new SwitchComponent());
        entity->AddComponent(new TextComponent());

        DVASSERT(entity->GetComponent<TransformComponent>()->GetType()->Is<TransformComponent>());
        DVASSERT(entity->GetComponent<LightComponent>()->GetType()->Is<LightComponent>());
        DVASSERT(entity->GetComponent<ActionComponent>()->GetType()->Is<ActionComponent>());
        DVASSERT(entity->GetComponent<AnimationComponent>()->GetType()->Is<AnimationComponent>());
        DVASSERT(entity->GetComponent<CameraComponent>()->GetType()->Is<CameraComponent>());
        DVASSERT(entity->GetComponent<CustomPropertiesComponent>()->GetType()->Is<CustomPropertiesComponent>());
        DVASSERT(entity->GetComponent<ParticleEffectComponent>()->GetType()->Is<ParticleEffectComponent>());
        DVASSERT(entity->GetComponent<SwitchComponent>()->GetType()->Is<SwitchComponent>());
        DVASSERT(entity->GetComponent<SwitchComponent>()->GetType()->Is<SwitchComponent>());

        mask |= ComponentUtils::MakeMask<LightComponent>();
        mask |= ComponentUtils::MakeMask<ActionComponent>();
        mask |= ComponentUtils::MakeMask<AnimationComponent>();
        mask |= ComponentUtils::MakeMask<CameraComponent>();
        mask |= ComponentUtils::MakeMask<CustomPropertiesComponent>();
        mask |= ComponentUtils::MakeMask<ParticleEffectComponent>();
        mask |= ComponentUtils::MakeMask<SwitchComponent>();
        mask |= ComponentUtils::MakeMask<TextComponent>();

        ComponentMask multiMask = ComponentUtils::MakeMask<TransformComponent, LightComponent, ActionComponent, AnimationComponent, CameraComponent,
                                                           CustomPropertiesComponent, ParticleEffectComponent, SwitchComponent, TextComponent>();

        TEST_VERIFY(multiMask == mask);

        multiMask = ComponentUtils::MakeMask(Type::Instance<TransformComponent>(), Type::Instance<LightComponent>(), Type::Instance<ActionComponent>(),
                                             Type::Instance<AnimationComponent>(), Type::Instance<CameraComponent>(), Type::Instance<CustomPropertiesComponent>(),
                                             Type::Instance<ParticleEffectComponent>(), Type::Instance<SwitchComponent>(), Type::Instance<TextComponent>());

        TEST_VERIFY(multiMask == mask);

        multiMask = ComponentUtils::MakeMask(Type::Instance<TransformComponent>(), Type::Instance<LightComponent>(), Type::Instance<ActionComponent>(),
                                             Type::Instance<AnimationComponent>(), Type::Instance<CameraComponent>(), Type::Instance<CustomPropertiesComponent>(),
                                             Type::Instance<ParticleEffectComponent>(), Type::Instance<SwitchComponent>());

        TEST_VERIFY(multiMask != mask);

        multiMask |= ComponentUtils::MakeMask<TextComponent>();

        TEST_VERIFY(multiMask == mask);

        TEST_VERIFY((entity->GetAvailableComponentMask() & mask) == mask);
        TEST_VERIFY(entity->GetAvailableComponentMask() == mask);

        ComponentManager* cm = GetEngineContext()->componentManager;

        for (const Type* type : GetEngineContext()->componentManager->GetRegisteredSceneComponents())
        {
            Component* c = entity->GetComponent(type);
            bool flagShouldBeSet = c != nullptr;
            TEST_VERIFY(mask.test(cm->GetRuntimeComponentId(type)) == flagShouldBeSet);
        }

        entity->RemoveComponent<TransformComponent>();

        ComponentMask ecm = entity->GetAvailableComponentMask();

        TEST_VERIFY(ecm != mask);
        TEST_VERIFY(ecm == (mask ^ ComponentUtils::MakeMask<TransformComponent>()));

        const Vector<const Type*> types = {
            Type::Instance<LightComponent>(), Type::Instance<ActionComponent>(),
            Type::Instance<AnimationComponent>(), Type::Instance<CameraComponent>(),
            Type::Instance<CustomPropertiesComponent>(), Type::Instance<ParticleEffectComponent>(),
            Type::Instance<SwitchComponent>(), Type::Instance<TextComponent>()
        };

        for (const Type* type : types)
        {
            TEST_VERIFY(entity->GetComponent(type) != nullptr);

            ComponentMask m = ComponentUtils::MakeMask(type);
            TEST_VERIFY((ecm & m) == m);

            TEST_VERIFY(entity->GetComponent(type)->GetType() == type);
        }

        SafeRelease(entity); // Components will be released in Entity destructor
    }

    DAVA_TEST (RegisterEntityTest)
    {
        Scene* scene = new Scene();
        SingleComponentSystem* testSystemLight = new SingleComponentSystem(scene);
        SingleComponentSystem* testSystemAction = new SingleComponentSystem(scene);
        scene->AddSystem(testSystemLight, ComponentUtils::MakeMask<LightComponent>());
        scene->AddSystem(testSystemAction, ComponentUtils::MakeMask<ActionComponent>());

        Entity* e1 = new Entity();
        e1->AddComponent(new LightComponent());
        e1->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        scene->AddNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 1);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 1);

        scene->RemoveNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        e1->Release();
        scene->Release();
    }

    DAVA_TEST (AddComponentTest1)
    {
        Scene* scene = new Scene();
        SingleComponentSystem* testSystemLight = new SingleComponentSystem(scene);
        SingleComponentSystem* testSystemAction = new SingleComponentSystem(scene);
        scene->AddSystem(testSystemLight, ComponentUtils::MakeMask<LightComponent>());
        scene->AddSystem(testSystemAction, ComponentUtils::MakeMask<ActionComponent>());

        Entity* e1 = new Entity();
        scene->AddNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        e1->AddComponent(new ActionComponent());
        e1->AddComponent(new LightComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 1);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 1);

        e1->AddComponent(new ActionComponent());
        e1->AddComponent(new LightComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 2);

        e1->RemoveComponent<ActionComponent>();
        e1->RemoveComponent<LightComponent>();

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 1);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 1);

        e1->AddComponent(new ActionComponent());
        e1->AddComponent(new LightComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 2);

        scene->RemoveNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        e1->Release();
        scene->Release();
    }

    DAVA_TEST (AddComponentTest2)
    {
        Scene* scene = new Scene();
        SingleComponentSystem* testSystemLight = new SingleComponentSystem(scene);
        SingleComponentSystem* testSystemAction = new SingleComponentSystem(scene);
        scene->AddSystem(testSystemLight, ComponentUtils::MakeMask<LightComponent>());
        scene->AddSystem(testSystemAction, ComponentUtils::MakeMask<ActionComponent>());

        Entity* e1 = new Entity();
        Component* a = new ActionComponent();
        Component* l = new LightComponent();

        scene->AddNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        e1->AddComponent(a);
        e1->AddComponent(l);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 1);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 1);

        e1->RemoveComponent(a);
        e1->RemoveComponent(l);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        a = l = NULL;
        scene->RemoveNode(e1);

        e1->Release();
        scene->Release();
    }

    DAVA_TEST (AddComponentTest3)
    {
        Scene* scene = new Scene();
        SingleComponentSystem* testSystemLight = new SingleComponentSystem(scene);
        SingleComponentSystem* testSystemAction = new SingleComponentSystem(scene);
        scene->AddSystem(testSystemLight, ComponentUtils::MakeMask<LightComponent>());
        scene->AddSystem(testSystemAction, ComponentUtils::MakeMask<ActionComponent>());

        Entity* e1 = new Entity();
        e1->AddComponent(new ActionComponent());
        e1->AddComponent(new LodComponent());
        e1->AddComponent(new LightComponent());
        e1->AddComponent(new LightComponent());

        Entity* e2 = new Entity();

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        scene->AddNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 1);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 2);

        e1->AddComponent(new ActionComponent());
        e1->AddComponent(new LightComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 3);

        scene->RemoveNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        scene->AddNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 3);

        scene->AddNode(e2);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 3);

        e2->AddComponent(new LodComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 3);

        e2->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 3);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 3);

        e2->AddComponent(new LightComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 3);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 4);

        e1->RemoveComponent<ActionComponent>();

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 4);

        e1->RemoveComponent<LightComponent>();

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 3);

        e1->AddComponent(new ActionComponent());
        e1->AddComponent(new LightComponent());

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 3);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 4);

        scene->RemoveNode(e1);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 1);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 1);

        scene->RemoveNode(e2);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 0);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 0);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 0);

        Entity* e3 = e1->Clone();

        scene->AddNode(e1);
        TEST_VERIFY(testSystemAction->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 2);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 1);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 3);

        scene->AddNode(e3);

        TEST_VERIFY(testSystemAction->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemAction->GetComponentsCount() == 4);
        TEST_VERIFY(testSystemLight->GetEnititesCount() == 2);
        TEST_VERIFY(testSystemLight->GetComponentsCount() == 6);

        e2->Release();
        e1->Release();
        scene->Release();
    }

    DAVA_TEST (MultiComponentTest1)
    {
        Scene* scene = new Scene();
        MultiComponentSystem* testSystem = new MultiComponentSystem(scene);
        scene->AddSystem(testSystem, ComponentUtils::MakeMask<LightComponent>() | ComponentUtils::MakeMask<ActionComponent>());

        Entity* e1 = new Entity();
        Component* a = new ActionComponent();
        Component* l = new LightComponent();

        scene->AddNode(e1);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(a);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(l);

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 1);

        e1->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 2);

        e1->RemoveComponent(a);

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 1);

        e1->RemoveComponent(l);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        a = l = NULL;

        scene->RemoveNode(e1);

        e1->Release();
        scene->Release();
    }

    DAVA_TEST (MultiComponentTest2)
    {
        Scene* scene = new Scene();
        MultiComponentSystem* testSystem = new MultiComponentSystem(scene);
        scene->AddSystem(testSystem, ComponentUtils::MakeMask<LightComponent>() | ComponentUtils::MakeMask<ActionComponent>());

        Entity* e1 = new Entity();
        Component* a = new ActionComponent();
        Component* l = new LightComponent();

        scene->AddNode(e1);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(a);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(l);

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 1);

        e1->RemoveComponent(a);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->RemoveComponent(l);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        a = l = NULL;

        scene->RemoveNode(e1);

        e1->Release();
        scene->Release();
    }

    DAVA_TEST (MultiComponentTest3)
    {
        Scene* scene = new Scene();
        MultiComponentSystem* testSystem = new MultiComponentSystem(scene);
        scene->AddSystem(testSystem, ComponentUtils::MakeMask<LightComponent>() | ComponentUtils::MakeMask<ActionComponent>());

        Entity* e1 = new Entity();

        scene->AddNode(e1);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(new LightComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 1);

        e1->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 2);

        e1->RemoveComponent<ActionComponent>();

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 1);

        e1->RemoveComponent<LightComponent>();

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e1->AddComponent(new LightComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 2);

        e1->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 3);

        e1->AddComponent(new ActionComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 4);

        Entity* e2 = new Entity();
        e2->AddComponent(new ActionComponent());
        e2->AddComponent(new ActionComponent());
        e2->AddComponent(new LightComponent());
        e2->AddComponent(new LightComponent());

        scene->AddNode(e2);

        TEST_VERIFY(testSystem->GetEnititesCount() == 2);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 3);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 6);

        e2->AddComponent(new ActionComponent());
        e2->AddComponent(new LightComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 2);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 4);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 7);

        e2->AddComponent(new LodComponent());

        TEST_VERIFY(testSystem->GetEnititesCount() == 2);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 4);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 7);

        scene->RemoveNode(e1);

        TEST_VERIFY(testSystem->GetEnititesCount() == 1);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 3);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 3);

        scene->RemoveNode(e2);

        TEST_VERIFY(testSystem->GetEnititesCount() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<LightComponent>() == 0);
        TEST_VERIFY(testSystem->GetComponentsCount<ActionComponent>() == 0);

        e2->Release();
        e1->Release();
        scene->Release();
    }
}
;
