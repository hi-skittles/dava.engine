#pragma once

#include <memory.h>

#include <TArc/Core/ClientModule.h>

#include <Reflection/Reflection.h>
#include <Base/GlobalEnum.h>

#include "Scene3D/Systems/ParticleEffectDebugDrawSystem.h"

namespace DAVA
{
class RenderObject;
class FieldBinder;
class SelectableGroup;
}

class ParticleDebugDrawModule : public DAVA::ClientModule
{
protected:
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;
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
    DAVA::UnorderedSet<DAVA::RenderObject*> ProcessSelection(const DAVA::SelectableGroup& group);
    std::shared_ptr<DAVA::FieldBinder> filedBinder;

    DAVA_VIRTUAL_REFLECTION(DAVA::ParticleDebugDrawModule, DAVA::ClientModule);
};