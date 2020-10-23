#include "Input/ActionSystem.h"
#include "Input/Private/ActionSystemImpl.h"

namespace DAVA
{
ActionSystem::ActionSystem()
    : impl(new Private::ActionSystemImpl(this))
{
}

ActionSystem::~ActionSystem()
{
    if (impl != nullptr)
    {
        delete impl;
        impl = nullptr;
    }
}

void ActionSystem::BindSet(const ActionSet& set, uint32 deviceId)
{
    impl->BindSet(set, Vector<uint32>{ deviceId });
}

void ActionSystem::BindSet(const ActionSet& set, uint32 deviceId1, uint32 deviceId2)
{
    impl->BindSet(set, Vector<uint32>{ deviceId1, deviceId2 });
}

void ActionSystem::UnbindAllSets()
{
    impl->UnbindAllSets();
}

bool ActionSystem::GetDigitalActionState(FastName actionId) const
{
    return impl->GetDigitalActionState(actionId);
}

AnalogActionState ActionSystem::GetAnalogActionState(FastName actionId) const
{
    return impl->GetAnalogActionState(actionId);
}
}
