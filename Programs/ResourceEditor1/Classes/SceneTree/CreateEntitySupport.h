#pragma once

#include <TArc/Qt/QtIcon.h>
#include <TArc/Qt/QtString.h>

#include <Base/RefPtr.h>
#include <Functional/Signal.h>
#include <Reflection/Reflection.h>

namespace DAVA
{
class Entity;
namespace TArc
{
class ContextAccessor;
class UI;
} // namespace TArc
} // namespace DAVA

class BaseEntityCreator : public DAVA::ReflectionBase
{
public:
    enum class eMenuPointOrder
    {
        UNKNOWN = -1,
        EMPTY_ENTITY,
        LIGHT_ENTITY,
        CAMERA_ENTITY,
        USER_NODE_ENITY,
        SWITCH_ENTITY,
        PARTICLE_EFFECT_ENTITY,
        LANDSCAPE_ENTITY,
        WIND_ENTITY,
        VEGETATION_ENTITY,
        PATH_ENTITY,
        TEXT_ENTITY,
        PHYSICS_ENTITIES,
        EDITOR_SPRITE
    };

    BaseEntityCreator(const QIcon& icon, const QString& text);
    virtual ~BaseEntityCreator() = default;

    virtual eMenuPointOrder GetOrder() const;
    void Init(DAVA::TArc::ContextAccessor* accessor, DAVA::TArc::UI* ui);

    QIcon icon;
    QString text;

protected:
    DAVA::TArc::ContextAccessor* accessor = nullptr;
    DAVA::TArc::UI* ui = nullptr;

    DAVA_VIRTUAL_REFLECTION(BaseEntityCreator);
};

class SceneEditor2;
class EntityCreator : public BaseEntityCreator
{
public:
    EntityCreator(const QIcon& icon, const QString& text);
    void StartEntityCreation(SceneEditor2* scene);

    virtual void Cancel();

    DAVA::Signal<> creationFinished;

protected:
    virtual void StartEntityCreationImpl() = 0;
    void FinishCreation();
    void AddEntity(DAVA::Entity* entity);

protected:
    SceneEditor2* scene = nullptr;

    DAVA_VIRTUAL_REFLECTION(EntityCreator, BaseEntityCreator);
};

class SimpleEntityCreator : public EntityCreator
{
public:
    SimpleEntityCreator(BaseEntityCreator::eMenuPointOrder order, const QIcon& icon, const QString& text,
                        const DAVA::Function<DAVA::RefPtr<DAVA::Entity>()>& fn);

    BaseEntityCreator::eMenuPointOrder GetOrder() const override;

protected:
    void StartEntityCreationImpl() override;

private:
    DAVA::Function<DAVA::RefPtr<DAVA::Entity>()> creationFn;
    BaseEntityCreator::eMenuPointOrder order;

    DAVA_VIRTUAL_REFLECTION(SimpleEntityCreator, EntityCreator);
};

template <typename T>
class SimpleCreatorHelper : public SimpleEntityCreator
{
public:
    SimpleCreatorHelper(BaseEntityCreator::eMenuPointOrder order, const QIcon& icon, const QString& text)
        : SimpleEntityCreator(order, icon, text, DAVA::MakeFunction(&T::CreateEntity))
    {
    }

    DAVA_VIRTUAL_REFLECTION(SimpleEntityCreator, EntityCreator);
};

class EntityCreatorsGroup : public BaseEntityCreator
{
public:
    EntityCreatorsGroup(const QIcon& icon, const QString& text);
    DAVA::Vector<std::unique_ptr<BaseEntityCreator>> creatorsGroup;

    DAVA_VIRTUAL_REFLECTION(EntityCreatorsGroup, BaseEntityCreator);
};

std::unique_ptr<BaseEntityCreator> CreateEntityCreationTree();