#pragma once

#include <REPlatform/DataNodes/Selectable.h>

#include <TArc/Core/OperationRegistrator.h>
#include <TArc/Core/OperationInvoker.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Qt/QtString.h>

#include <Base/Any.h>
#include <Base/BaseTypes.h>
#include <Base/FastName.h>
#include <Command/Command.h>
#include <FileSystem/FilePath.h>
#include <Functional/Signal.h>
#include <Reflection/ReflectedType.h>

namespace DAVA
{
class SceneEditor2;
class ParticleEffectComponent;
class ParticleEmitter;
class ContextAccessor;
class UI;
class OperationInvoker;
} // namespace DAVA

class SceneTreeModelV2;

class QMenu;
class QPoint;
class QAction;

class BaseContextMenu
{
public:
    BaseContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject);

    void Init(DAVA::ContextAccessor* accessor, DAVA::UI* ui, DAVA::OperationInvoker* invoker);
    void Show(const QPoint& pos);

protected:
    virtual void FillActions(QMenu& menu) = 0;

    void RemoveCommandsHelper(const DAVA::String& text, const DAVA::ReflectedType* type, const DAVA::Function<std::unique_ptr<DAVA::Command>(const DAVA::Selectable&)>& callback);
    void ForEachSelectedByType(const DAVA::ReflectedType* type, const DAVA::Function<void(const DAVA::Selectable&)>& callback);

    DAVA::ParticleEffectComponent* GetParticleEffectComponent(const DAVA::Selectable& object);
    void SaveEmitter(DAVA::ParticleEffectComponent* component, DAVA::ParticleEmitter* emitter,
                     bool askFileName, const QString& defaultName,
                     const DAVA::Function<std::unique_ptr<DAVA::Command>(const DAVA::FilePath&)>& commandCreator);

    void Connect(QAction* action, const DAVA::Function<void()>& fn);
    QString GetParticlesConfigPath();
    DAVA::FilePath GetDataSourcePath() const;

protected:
    DAVA::SceneEditor2* scene = nullptr;
    const SceneTreeModelV2* model = nullptr;
    DAVA::Vector<DAVA::Selectable> selectedObjects;
    DAVA::Selectable currentObject;

    DAVA::ContextAccessor* accessor = nullptr;
    DAVA::UI* ui = nullptr;
    DAVA::OperationInvoker* invoker = nullptr;

    DAVA::QtConnections connections;
};

class EntityContextMenu : public BaseContextMenu
{
    using TBase = BaseContextMenu;

public:
    EntityContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject);

protected:
    void FillActions(QMenu& menu) override;

private:
    void FillCameraActions(QMenu& menu);

    void SaveEntityAs();
    void EditModel();
    void ReloadModel();
    void ReloadModelAs();
    void ReloadTexturesInSelected();

    void AddEmitter();
    void SaveEffectEmitters();
    void SaveEffectEmittersAs();
    void GrabImage();

    void PerformSaveEffectEmitters(bool forceAskFileName);

    template <typename CMD, typename... Arg>
    void ExecuteCommandForEffect(Arg&&... args);

    void StartEffect();
    void StopEffect();
    void RestartEffect();

    void SetEntityNameAsFilter();

    void SetCurrentCamera();
    void SetCustomDrawCamera();

    void SaveSlotsPreset();
    void LoadSlotsPreset();
};

class ParticleLayerContextMenu : public BaseContextMenu
{
    using TBase = BaseContextMenu;

public:
    ParticleLayerContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject);

protected:
    void FillActions(QMenu& menu) override;

private:
    void CloneLayer();
    void RemoveLayer();
    void AddForce();
    void AddDrag();
    void AddVortex();
    void AddGravity();
    void AddWind();
    void AddPointGravity();
    void AddPlaneCollision();
};

class ParticleSimplifiedForceContextMenu : public BaseContextMenu
{
    using TBase = BaseContextMenu;

public:
    ParticleSimplifiedForceContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject);

protected:
    void FillActions(QMenu& menu) override;

private:
    void RemoveForce();
};

class ParticleForceContextMenu : public BaseContextMenu
{
    //////////////////////////////////////////////////////////////////////////
    using TBase = BaseContextMenu;

public:
    ParticleForceContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject);

protected:
    void FillActions(QMenu& menu) override;

private:
    void CloneForce();
    void RemoveForce();
};

class ParticleEmitterContextMenu : public BaseContextMenu
{
    using TBase = BaseContextMenu;

public:
    ParticleEmitterContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject);

protected:
    void FillActions(QMenu& menu) override;

    void RemoveEmitter();
    void AddLayer();
    void LoadEmitterFromYaml();
    void SaveEmitterToYaml();
    void SaveEmitterToYamlAs();
    void SaveEmitterLocal(bool forceAskFileName);

    std::unique_ptr<DAVA::Command> CreateLoadCommand(const DAVA::FilePath& path);
    std::unique_ptr<DAVA::Command> CreateSaveCommand(const DAVA::FilePath& path);
};

DECLARE_OPERATION_ID(SetSceneTreeFilter);
DECLARE_OPERATION_ID(ReloadTexturesInSelectedOperation);

std::unique_ptr<BaseContextMenu> CreateSceneTreeContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject);
