#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"

namespace DAVA
{
class BaseObject;

class IUpdatable
{
public:
    virtual ~IUpdatable(){};
};

class IUpdatableBeforeTransform : public virtual IUpdatable
{
public:
    virtual void UpdateBeforeTransform(float32 timeElapsed) = 0;
};

class IUpdatableAfterTransform : public virtual IUpdatable
{
public:
    virtual void UpdateAfterTransform(float32 timeElapsed) = 0;
};

class UpdatableComponent : public Component
{
public:
    enum eUpdateType
    {
        UPDATE_PRE_TRANSFORM,
        UPDATE_POST_TRANSFORM,

        UPDATES_COUNT
    };

protected:
    ~UpdatableComponent(){};

public:
    UpdatableComponent();

    Component* Clone(Entity* toEntity) override;

    void SetUpdatableObject(IUpdatable* updatableObject);
    IUpdatable* GetUpdatableObject();

private:
    IUpdatable* updatableObject;

    DAVA_VIRTUAL_REFLECTION(UpdatableComponent, Component);
};
}
