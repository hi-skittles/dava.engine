#pragma once

#include <memory.h>

#include <TArc/Core/ClientModule.h>

#include <Reflection/Reflection.h>
#include <Base/GlobalEnum.h>

#include "Scene3D/Systems/ParticleEffectDebugDrawSystem.h"

namespace DAVA
{
class RenderObject;

namespace TArc
{
class FieldBinder;
}
}

class SelectableGroup;

class ParticleDebugDrawModule : public DAVA::TArc::ClientModule
{
protected:
    void OnContextCreated(DAVA::TArc::DataContext* context) override;
    void OnContextDeleted(DAVA::TArc::DataContext* context) override;
    void PostInit() override;

    void OnSelectionChanged(const DAVA::Any selection);

private:
    bool GetSystemEnabledState() const;
    void SetSystemEnabledState(bool enabled);

    bool GetDrawOnlySelected() const;
    void SetDrawOnlySelected(bool drawOnlySelected);

    bool IsDisabled() const;

    DAVA::eParticleDebugDrawMode GetDrawMode() const;
    void SetDrawMode(DAVA::eParticleDebugDrawMode drawMode);

    void UpdateSceneSystem();
    DAVA::UnorderedSet<DAVA::RenderObject*> ProcessSelection(const SelectableGroup& group);
    std::shared_ptr<DAVA::TArc::FieldBinder> filedBinder;

    DAVA_VIRTUAL_REFLECTION(DAVA::ParticleDebugDrawModule, DAVA::TArc::ClientModule);
};