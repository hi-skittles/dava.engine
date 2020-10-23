#include "Classes/SceneTree/Private/SceneTreeContextMenu.h"
#include "Classes/SceneTree/Private/SceneTreeModelV2.h"
#include "Classes/Qt/Tools/PathDescriptor/PathDescriptor.h"
#include "Classes/Qt/Actions/SaveEntityAsAction.h"
#include "Classes/Qt/Scene/SceneImageGraber.h"

#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/Settings/GlobalSceneSettings.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/DataNodes/Settings/SlotSystemSettings.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Global/StringConstants.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/SceneHelper.h>
#include <REPlatform/Scene/Systems/EditorParticlesSystem.h>
#include <REPlatform/Scene/Systems/EditorSlotSystem.h>
#include <REPlatform/Scene/Systems/StructureSystem.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Utils/Utils.h>
#include <TArc/WindowSubSystem/UI.h>

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Reflection/ReflectedTypeDB.h>

#include <QMenu>
#include <QModelIndex>
#include <QPoint>
#include <QBoxLayout>
#include <QDialog>
#include <QCheckBox>
#include <QDialogButtonBox>

BaseContextMenu::BaseContextMenu(DAVA::SceneEditor2* scene_, const SceneTreeModelV2* model_, const DAVA::Vector<DAVA::Selectable>& selectedObjects_, const DAVA::Selectable& currentObject_)
    : scene(scene_)
    , model(model_)
    , selectedObjects(selectedObjects_)
    , currentObject(currentObject_)
{
}

void BaseContextMenu::Init(DAVA::ContextAccessor* accessor_, DAVA::UI* ui_, DAVA::OperationInvoker* invoker_)
{
    accessor = accessor_;
    ui = ui_;
    invoker = invoker_;
}

void BaseContextMenu::Show(const QPoint& pos)
{
    QMenu menu;
    FillActions(menu);
    menu.exec(pos);
}

void BaseContextMenu::RemoveCommandsHelper(const DAVA::String& text, const DAVA::ReflectedType* type, const DAVA::Function<std::unique_ptr<DAVA::Command>(const DAVA::Selectable&)>& callback)
{
    DAVA::Set<DAVA::Selectable> newSelection(selectedObjects.begin(), selectedObjects.end());
    DAVA::Set<DAVA::Selectable> objectToRemove;
    for (const DAVA::Selectable& obj : selectedObjects)
    {
        if (obj.GetObjectType() == type)
        {
            objectToRemove.insert(obj);
            newSelection.erase(obj);
        }
    }
    DAVA::SelectableGroup newSelectionGroup;
    newSelectionGroup.Add(DAVA::Vector<DAVA::Selectable>(newSelection.begin(), newSelection.end()));
    DAVA::SelectionData* selectionData = accessor->GetActiveContext()->GetData<DAVA::SelectionData>();
    DVASSERT(selectionData != nullptr);
    selectionData->SetSelection(newSelectionGroup);

    auto iter = objectToRemove.begin();
    while (iter != objectToRemove.end())
    {
        bool validForRemove = true;
        QModelIndex index = model->GetIndexByObject(*iter).parent();
        while (index.isValid())
        {
            DAVA::Selectable parentObj = model->GetObjectByIndex(index);
            if (objectToRemove.count(parentObj) > 0)
            {
                validForRemove = false;
                break;
            }

            index = index.parent();
        }

        if (validForRemove == false)
        {
            iter = objectToRemove.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    if (objectToRemove.empty() == true)
    {
        return;
    }

    scene->BeginBatch(text, static_cast<DAVA::uint32>(objectToRemove.size()));
    DAVA::Vector<std::unique_ptr<DAVA::Command>> commands;
    commands.reserve(selectedObjects.size());
    for (const DAVA::Selectable& obj : objectToRemove)
    {
        commands.push_back(callback(obj));
    }
    for (std::unique_ptr<DAVA::Command>& cmd : commands)
    {
        scene->Exec(std::move(cmd));
    }
    scene->EndBatch();
}

void BaseContextMenu::ForEachSelectedByType(const DAVA::ReflectedType* type, const DAVA::Function<void(const DAVA::Selectable&)>& callback)
{
    for (const DAVA::Selectable& obj : selectedObjects)
    {
        if (obj.GetObjectType() == type)
        {
            callback(obj);
        }
    }
}

DAVA::ParticleEffectComponent* BaseContextMenu::GetParticleEffectComponent(const DAVA::Selectable& object)
{
    DAVA::Selectable objectToCheck = object;
    QModelIndex objectIndex = model->GetIndexByObject(objectToCheck);
    while (objectIndex.isValid() == true)
    {
        if (objectToCheck.CanBeCastedTo<DAVA::Entity>() == true)
        {
            DAVA::Entity* entity = objectToCheck.Cast<DAVA::Entity>();
            DAVA::ParticleEffectComponent* component = DAVA::GetEffectComponent(entity);
            DVASSERT(component != nullptr);
            return component;
        }

        objectIndex = objectIndex.parent();
        objectToCheck = model->GetObjectByIndex(objectIndex);
    }

    DVASSERT(false);
    return nullptr;
}

void BaseContextMenu::SaveEmitter(DAVA::ParticleEffectComponent* component, DAVA::ParticleEmitter* emitter,
                                  bool askFileName, const QString& defaultName,
                                  const DAVA::Function<std::unique_ptr<DAVA::Command>(const DAVA::FilePath&)>& commandCreator)
{
    askFileName |= emitter->configPath.IsEmpty();

    DAVA::FilePath yamlPath = emitter->configPath;
    if (askFileName)
    {
        DAVA::CommonInternalSettings* settings = accessor->GetGlobalContext()->GetData<DAVA::CommonInternalSettings>();
        DAVA::FilePath defaultPath = settings->emitterSaveDir;
        QString particlesPath = defaultPath.IsEmpty() ? GetParticlesConfigPath() : QString::fromStdString(defaultPath.GetAbsolutePathname());

        DAVA::GetEngineContext()->fileSystem->CreateDirectory(DAVA::FilePath(particlesPath.toStdString()), true); //to ensure that folder is created

        DAVA::FileDialogParams params;
        params.dir = particlesPath + defaultName;
        params.filters = QStringLiteral("YAML File (*.yaml)");
        params.title = QStringLiteral("Save Particle Emitter ") + QString(emitter->name.c_str());

        QString filePath = ui->GetSaveFileName(DAVA::mainWindowKey, params);
        if (filePath.isEmpty())
        {
            return;
        }

        yamlPath = DAVA::FilePath(filePath.toStdString());
        settings->emitterSaveDir = yamlPath.GetDirectory();
    }

    scene->Exec(commandCreator(yamlPath));
    if (askFileName)
    {
        scene->SetChanged();
    }
}

void BaseContextMenu::Connect(QAction* action, const DAVA::Function<void()>& fn)
{
    connections.AddConnection(action, &QAction::triggered, fn);
}

QString BaseContextMenu::GetParticlesConfigPath()
{
    DAVA::ProjectManagerData* data = accessor->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
    if (data == nullptr)
        return QString("");

    return QString::fromStdString(data->GetParticlesConfigPath().GetAbsolutePathname());
}

DAVA::FilePath BaseContextMenu::GetDataSourcePath() const
{
    DAVA::ProjectManagerData* data = accessor->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
    if (data == nullptr)
        return DAVA::FilePath();

    return data->GetDataSource3DPath();
}

EntityContextMenu::EntityContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject)
    : BaseContextMenu(scene, model, selectedObjects, currentObject)
{
}

void EntityContextMenu::FillActions(QMenu& menu)
{
    DAVA::Entity* entity = currentObject.Cast<DAVA::Entity>();
    DAVA::Camera* camera = GetCamera(entity);
    DAVA::uint32 selectionSize = static_cast<DAVA::uint32>(selectedObjects.size());

    DAVA::SceneEditor2* scene = this->scene;
    if (entity->GetName().find("editor.") != DAVA::String::npos)
    {
        if (selectionSize == 1 && camera != nullptr)
        {
            FillCameraActions(menu);
            menu.addSeparator();
        }

        if ((camera != scene->GetCurrentCamera()) && (entity->GetNotRemovable() == false))
        {
            Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/remove.png"), QStringLiteral("Remove entity")), [scene] { DAVA::RemoveSelection(scene); });
        }
    }
    else
    {
        Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/zoom.png"), QStringLiteral("Look at")), [scene] { LookAtSelection(scene); });
        if (camera != nullptr)
        {
            FillCameraActions(menu);
        }

        menu.addSeparator();
        if (entity->GetLocked() == false && (camera != scene->GetCurrentCamera()) && (entity->GetNotRemovable() == false))
        {
            Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/remove.png"), QStringLiteral("Remove entity")), [scene] { DAVA::RemoveSelection(scene); });
        }

        menu.addSeparator();
        QAction* lockAction = menu.addAction(DAVA::SharedIcon(":/QtIcons/lock_add.png"), QStringLiteral("Lock"));
        QAction* unlockAction = menu.addAction(DAVA::SharedIcon(":/QtIcons/lock_delete.png"), QStringLiteral("Unlock"));
        Connect(lockAction, [scene] { LockTransform(scene); });
        Connect(unlockAction, [scene] { UnlockTransform(scene); });
        if (entity->GetLocked())
        {
            lockAction->setDisabled(true);
        }
        else
        {
            unlockAction->setDisabled(true);
        }

        menu.addSeparator();
        Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/save_as.png"), QStringLiteral("Save Entity As...")), DAVA::MakeFunction(this, &EntityContextMenu::SaveEntityAs));

        DAVA::KeyedArchive* customProp = GetCustomPropertiesArchieve(entity);
        bool isConstReference = false;
        if (nullptr != customProp)
        {
            DAVA::FilePath ownerRef = customProp->GetString(DAVA::ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
            if (!ownerRef.IsEmpty())
            {
                if (selectionSize == 1)
                {
                    Connect(menu.addAction(QStringLiteral("Edit Model")), DAVA::MakeFunction(this, &EntityContextMenu::EditModel));
                }

                Connect(menu.addAction(QStringLiteral("Reload Model...")), DAVA::MakeFunction(this, &EntityContextMenu::ReloadModel));
            }

            isConstReference = customProp->GetBool(DAVA::ResourceEditor::EDITOR_CONST_REFERENCE, false);
        }

        if (isConstReference != true)
        {
            Connect(menu.addAction(QStringLiteral("Reload Model As...")), DAVA::MakeFunction(this, &EntityContextMenu::ReloadModelAs));
        }

        // Reload textures in selection
        {
            bool hasTextures = false;

            for (DAVA::Selectable obj : selectedObjects)
            {
                if (obj.CanBeCastedTo<DAVA::Entity>())
                {
                    DAVA::Entity* e = obj.Cast<DAVA::Entity>();
                    DAVA::SceneHelper::TextureCollector collector;
                    DAVA::SceneHelper::EnumerateEntityTextures(scene, e, collector);
                    if (collector.GetTextures().empty() == false)
                    {
                        hasTextures = true;
                        break;
                    }
                }
            }

            if (hasTextures == true)
            {
                Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/reloadtextures.png"), QStringLiteral("Reload Textures")),
                        DAVA::MakeFunction(this, &EntityContextMenu::ReloadTexturesInSelected));
            }
        }

        // particle effect
        DAVA::ParticleEffectComponent* effect = DAVA::GetEffectComponent(entity);
        if (nullptr != effect)
        {
            menu.addSeparator();
            QMenu* particleEffectMenu = menu.addMenu("Particle Effect");

            Connect(particleEffectMenu->addAction(DAVA::SharedIcon(":/QtIcons/emitter_particle.png"), QStringLiteral("Add Emitter")), DAVA::MakeFunction(this, &EntityContextMenu::AddEmitter));
            Connect(particleEffectMenu->addAction(DAVA::SharedIcon(":/QtIcons/savescene.png"), QStringLiteral("Save Effect Emitters")), DAVA::MakeFunction(this, &EntityContextMenu::SaveEffectEmitters));
            Connect(particleEffectMenu->addAction(DAVA::SharedIcon(":/QtIcons/savescene.png"), QStringLiteral("Save Effect Emitters As...")), DAVA::MakeFunction(this, &EntityContextMenu::SaveEffectEmittersAs));
            particleEffectMenu->addSeparator();
            Connect(particleEffectMenu->addAction(DAVA::SharedIcon(":/QtIcons/play.png"), QStringLiteral("Start")), DAVA::MakeFunction(this, &EntityContextMenu::StartEffect));
            Connect(particleEffectMenu->addAction(DAVA::SharedIcon(":/QtIcons/stop.png"), QStringLiteral("Stop")), DAVA::MakeFunction(this, &EntityContextMenu::StopEffect));
            Connect(particleEffectMenu->addAction(DAVA::SharedIcon(":/QtIcons/restart.png"), QStringLiteral("Restart")), DAVA::MakeFunction(this, &EntityContextMenu::RestartEffect));
        }

        if (selectionSize == 1)
        {
            menu.addSeparator();
            Connect(menu.addAction(DAVA::SharedIcon(":/QtIconsTextureDialog/filter.png"), QStringLiteral("Set name as filter")), DAVA::MakeFunction(this, &EntityContextMenu::SetEntityNameAsFilter));
        }

        menu.addSeparator();
        {
            Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/openscene.png"), QStringLiteral("Load slots preset")), DAVA::MakeFunction(this, &EntityContextMenu::LoadSlotsPreset));
        }
        {
            std::function<bool(DAVA::Entity*)> checkSlotComponent = [&checkSlotComponent](DAVA::Entity* entity) {
                if (entity->GetComponentCount<DAVA::SlotComponent>() > 0)
                    return true;

                for (DAVA::int32 i = 0; i < entity->GetChildrenCount(); ++i)
                {
                    if (checkSlotComponent(entity->GetChild(i)) == true)
                        return true;
                }

                return false;
            };
            bool hasSlotComponent = checkSlotComponent(entity);
            QAction* action = menu.addAction(DAVA::SharedIcon(":/QtIcons/save_as.png"), QStringLiteral("Save slots preset"));
            action->setEnabled(hasSlotComponent);
            Connect(action, DAVA::MakeFunction(this, &EntityContextMenu::SaveSlotsPreset));
        }
    }
}

void EntityContextMenu::FillCameraActions(QMenu& menu)
{
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/eye.png"), QStringLiteral("Look from")), DAVA::MakeFunction(this, &EntityContextMenu::SetCurrentCamera));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/camera.png"), QStringLiteral("Set custom draw camera")), DAVA::MakeFunction(this, &EntityContextMenu::SetCustomDrawCamera));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/grab-image.png"), QStringLiteral("Grab image")), DAVA::MakeFunction(this, &EntityContextMenu::GrabImage));
}

void EntityContextMenu::SaveEntityAs()
{
    DVASSERT(selectedObjects.empty() == false);

    DAVA::FilePath scenePath = scene->GetScenePath().GetDirectory();
    if (!DAVA::GetEngineContext()->fileSystem->Exists(scenePath) || !scene->IsLoaded())
    {
        scenePath = GetDataSourcePath();
    }

    DAVA::FileDialogParams params;
    params.dir = QString::fromStdString(scenePath.GetDirectory().GetAbsolutePathname());
    params.filters = QStringLiteral("DAVA SceneV2 (*.sc2)");
    params.title = QStringLiteral("Save scene file");

    QString filePath = ui->GetSaveFileName(DAVA::mainWindowKey, params);
    if (!filePath.isEmpty())
    {
        DAVA::SelectableGroup group;
        group.Add(selectedObjects);
        SaveEntityAsAction saver(&group, filePath.toStdString());
        saver.Run();
    }
}

void EntityContextMenu::EditModel()
{
    DAVA::SelectionData* selectionData = accessor->GetActiveContext()->GetData<DAVA::SelectionData>();
    const DAVA::SelectableGroup& selection = selectionData->GetSelection();
    for (auto entity : selection.ObjectsOfType<DAVA::Entity>())
    {
        DAVA::KeyedArchive* archive = GetCustomPropertiesArchieve(entity);
        if (archive)
        {
            DAVA::FilePath entityRefPath = archive->GetString(DAVA::ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
            if (DAVA::FileSystem::Instance()->Exists(entityRefPath))
            {
                invoker->Invoke(DAVA::OpenSceneOperation.ID, entityRefPath);
            }
            else
            {
                DAVA::Logger::Error((DAVA::ResourceEditor::SCENE_TREE_WRONG_REF_TO_OWNER + entityRefPath.GetAbsolutePathname()).c_str());
            }
        }
    }
}

void EntityContextMenu::ReloadModel()
{
    QDialog dlg;

    QVBoxLayout* dlgLayout = new QVBoxLayout();
    dlgLayout->setMargin(10);

    dlg.setWindowTitle("Reload Model options");
    dlg.setLayout(dlgLayout);

    QCheckBox* lightmapsChBox = new QCheckBox(QStringLiteral("Leave lightmap settings"), &dlg);
    dlgLayout->addWidget(lightmapsChBox);
    lightmapsChBox->setCheckState(Qt::Checked);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
    dlgLayout->addWidget(buttons);

    QObject::connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));

    if (QDialog::Accepted == ui->ShowModalDialog(DAVA::mainWindowKey, &dlg))
    {
        DAVA::String wrongPathes;
        DAVA::SelectionData* selectionData = accessor->GetActiveContext()->GetData<DAVA::SelectionData>();
        const DAVA::SelectableGroup& selection = selectionData->GetSelection();
        for (auto entity : selection.ObjectsOfType<DAVA::Entity>())
        {
            DAVA::KeyedArchive* archive = GetCustomPropertiesArchieve(entity);
            if (archive)
            {
                DAVA::FilePath pathToReload(archive->GetString(DAVA::ResourceEditor::EDITOR_REFERENCE_TO_OWNER));
                if (!DAVA::FileSystem::Instance()->Exists(pathToReload))
                {
                    wrongPathes += DAVA::Format("\r\n%s : %s", entity->GetName().c_str(),
                                                pathToReload.GetAbsolutePathname().c_str());
                }
            }
        }
        if (!wrongPathes.empty())
        {
            DAVA::Logger::Error((DAVA::ResourceEditor::SCENE_TREE_WRONG_REF_TO_OWNER + wrongPathes).c_str());
        }
        DAVA::SelectableGroup newSelection = scene->GetSystem<DAVA::StructureSystem>()->ReloadEntities(selection, lightmapsChBox->isChecked());
        selectionData->SetSelection(newSelection);
    }
}

void EntityContextMenu::ReloadModelAs()
{
    DAVA::SelectionData* selectionData = accessor->GetActiveContext()->GetData<DAVA::SelectionData>();
    const DAVA::SelectableGroup& selection = selectionData->GetSelection();
    DAVA::Entity* entity = selection.GetContent().front().AsEntity();
    DAVA::KeyedArchive* archive = GetCustomPropertiesArchieve(entity);
    if (archive != nullptr)
    {
        DAVA::String ownerPath = archive->GetString(DAVA::ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
        if (ownerPath.empty())
        {
            DAVA::FilePath p = scene->GetScenePath().GetDirectory();
            if (DAVA::FileSystem::Instance()->Exists(p) && scene->IsLoaded())
            {
                ownerPath = p.GetAbsolutePathname();
            }
            else
            {
                ownerPath = GetDataSourcePath().GetAbsolutePathname();
            }
        }

        DAVA::FileDialogParams params;
        params.dir = QString::fromStdString(ownerPath);
        params.filters = QStringLiteral("DAVA SceneV2 (*.sc2)");
        params.title = QStringLiteral("Open scene file");

        QString filePath = ui->GetOpenFileName(DAVA::mainWindowKey, params);
        if (!filePath.isEmpty())
        {
            DAVA::SelectableGroup newSelection = scene->GetSystem<DAVA::StructureSystem>()->ReloadEntitiesAs(selection, filePath.toStdString());
            selectionData->SetSelection(newSelection);
        }
    }
}

void EntityContextMenu::ReloadTexturesInSelected()
{
    invoker->Invoke(ReloadTexturesInSelectedOperation.ID);
}

void EntityContextMenu::AddEmitter()
{
    DAVA::Entity* entity = currentObject.Cast<DAVA::Entity>();
    DVASSERT(DAVA::GetEffectComponent(entity) != nullptr);

    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticleEmitter(entity)));
}

void EntityContextMenu::SaveEffectEmitters()
{
    PerformSaveEffectEmitters(false);
}

void EntityContextMenu::SaveEffectEmittersAs()
{
    PerformSaveEffectEmitters(true);
}

void EntityContextMenu::GrabImage()
{
    DAVA::FilePath scenePath = scene->GetScenePath();
    DAVA::FileDialogParams params;
    params.dir = QString::fromStdString(scenePath.GetDirectory().GetAbsolutePathname());
    params.filters = PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter;
    params.title = QStringLiteral("Save Scene Image");

    QString filePath = ui->GetSaveFileName(DAVA::mainWindowKey, params);

    if (filePath.isEmpty())
        return;

    SceneImageGrabber::Params grabParams;
    grabParams.scene = scene;
    grabParams.cameraToGrab = GetCamera(currentObject.Cast<DAVA::Entity>());
    DVASSERT(grabParams.cameraToGrab.Get() != nullptr);

    DAVA::GlobalSceneSettings* settings = accessor->GetGlobalContext()->GetData<DAVA::GlobalSceneSettings>();
    grabParams.imageSize = DAVA::Size2i(settings->grabSizeWidth, settings->grabSizeHeight);
    grabParams.outputFile = filePath.toStdString();

    SceneImageGrabber::GrabImage(grabParams);
}

void EntityContextMenu::PerformSaveEffectEmitters(bool forceAskFileName)
{
    DAVA::Entity* entity = currentObject.Cast<DAVA::Entity>();
    DAVA::ParticleEffectComponent* effect = DAVA::GetEffectComponent(entity);
    DVASSERT(effect != nullptr);

    QString effectName(entity->GetName().c_str());
    for (DAVA::int32 i = 0, sz = effect->GetEmittersCount(); i != sz; ++i)
    {
        DAVA::ParticleEmitterInstance* instance = effect->GetEmitterInstance(i);
        QString defName = effectName + "_" + QString::number(i + 1) + "_" + QString(instance->GetEmitter()->name.c_str()) + ".yaml";
        SaveEmitter(effect, instance->GetEmitter(), forceAskFileName, defName, [&](const DAVA::FilePath& path) {
            return std::unique_ptr<DAVA::Command>(new DAVA::CommandSaveParticleEmitterToYaml(effect, instance, path));
        });
    }
}

template <typename CMD, typename... Arg>
void EntityContextMenu::ExecuteCommandForEffect(Arg&&... args)
{
    for (const DAVA::Selectable& obj : selectedObjects)
    {
        if (obj.CanBeCastedTo<DAVA::Entity>() == false)
        {
            continue;
        }
        DAVA::Entity* entity = obj.Cast<DAVA::Entity>();
        DAVA::ParticleEffectComponent* effect = DAVA::GetEffectComponent(entity);
        if (nullptr != effect)
        {
            scene->Exec(std::unique_ptr<CMD>(new CMD(entity, std::forward<Arg>(args)...)));
        }
    }
}

void EntityContextMenu::StartEffect()
{
    ExecuteCommandForEffect<DAVA::CommandStartStopParticleEffect>(true);
}

void EntityContextMenu::StopEffect()
{
    ExecuteCommandForEffect<DAVA::CommandStartStopParticleEffect>(false);
}

void EntityContextMenu::RestartEffect()
{
    ExecuteCommandForEffect<DAVA::CommandRestartParticleEffect>();
}

void EntityContextMenu::SetEntityNameAsFilter()
{
    DAVA::SelectionData* selectionData = accessor->GetActiveContext()->GetData<DAVA::SelectionData>();
    const DAVA::SelectableGroup& selection = selectionData->GetSelection();
    DVASSERT(selection.GetSize() == 1);

    invoker->Invoke(SetSceneTreeFilter.ID, DAVA::String(currentObject.Cast<DAVA::Entity>()->GetName().c_str()));
}

void EntityContextMenu::SetCurrentCamera()
{
    DAVA::Camera* camera = GetCamera(currentObject.Cast<DAVA::Entity>());
    DVASSERT(camera != nullptr);
    scene->SetCurrentCamera(camera);
}

void EntityContextMenu::SetCustomDrawCamera()
{
    DAVA::Camera* camera = GetCamera(currentObject.Cast<DAVA::Entity>());
    DVASSERT(camera != nullptr);
    scene->SetCustomDrawCamera(camera);
}

void EntityContextMenu::SaveSlotsPreset()
{
    DAVA::SlotSystemSettings* settings = accessor->GetGlobalContext()->GetData<DAVA::SlotSystemSettings>();

    if (scene->IsLoaded() == false)
    {
        DAVA::Logger::Error("[SaveSlotsPreset] Sorry, you should save scene first");
        return;
    }

    DAVA::FileDialogParams params;
    params.dir = settings->lastPresetSaveLoadPath;
    params.filters = QStringLiteral("Slots preset file (*.spreset)");
    params.title = QStringLiteral("Save slot preset");

    QString selectedPath = ui->GetSaveFileName(DAVA::mainWindowKey, params);
    if (selectedPath.isEmpty())
    {
        return;
    }

    settings->lastPresetSaveLoadPath = selectedPath;
    DAVA::EditorSlotSystem* system = scene->GetSystem<DAVA::EditorSlotSystem>();
    DAVA::RefPtr<DAVA::KeyedArchive> archive = system->SaveSlotsPreset(currentObject.Cast<DAVA::Entity>());
    if (archive->Save(selectedPath.toStdString()) == false)
    {
        DAVA::Logger::Error("Slots Preset was not saved");
    }
}

void EntityContextMenu::LoadSlotsPreset()
{
    DAVA::SlotSystemSettings* settings = accessor->GetGlobalContext()->GetData<DAVA::SlotSystemSettings>();

    DAVA::FileDialogParams params;
    params.dir = settings->lastPresetSaveLoadPath;
    params.filters = QStringLiteral("Slots preset file (*.spreset)");
    params.title = QStringLiteral("Load slot preset");

    QString selectedPath = ui->GetOpenFileName(DAVA::mainWindowKey, params);
    if (selectedPath.isEmpty())
    {
        return;
    }

    settings->lastPresetSaveLoadPath = selectedPath;
    DAVA::RefPtr<DAVA::KeyedArchive> archive(new DAVA::KeyedArchive());
    if (archive->Load(selectedPath.toStdString()) == false)
    {
        DAVA::Logger::Error("Can't read selected slots preset");
        return;
    }

    DAVA::EditorSlotSystem* system = scene->GetSystem<DAVA::EditorSlotSystem>();
    system->LoadSlotsPreset(currentObject.Cast<DAVA::Entity>(), archive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

ParticleLayerContextMenu::ParticleLayerContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject)
    : BaseContextMenu(scene, model, selectedObjects, currentObject)
{
}

void ParticleLayerContextMenu::FillActions(QMenu& menu)
{
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/clone.png"), QStringLiteral("Clone Layer")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::CloneLayer));
    QString removeLayerText = selectedObjects.size() < 2 ? QStringLiteral("Remove Layer") : QStringLiteral("Remove Layers");
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/remove_layer.png"), removeLayerText), DAVA::MakeFunction(this, &ParticleLayerContextMenu::RemoveLayer));
    menu.addSeparator();
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/force.png"), QStringLiteral("Add Force")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::AddForce));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/turtle.png"), QStringLiteral("Add Drag")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::AddDrag));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/vortex_ico.png"), QStringLiteral("Add Vortex")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::AddVortex));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/gravity.png"), QStringLiteral("Add Gravity")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::AddGravity));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/wind_p.png"), QStringLiteral("Add Wind")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::AddWind));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/pointGravity.png"), QStringLiteral("Add Point Gravity")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::AddPointGravity));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/plane_coll.png"), QStringLiteral("Add Plane Collision")), DAVA::MakeFunction(this, &ParticleLayerContextMenu::AddPlaneCollision));
}

void ParticleLayerContextMenu::CloneLayer()
{
    DAVA::ParticleLayer* layer = currentObject.Cast<DAVA::ParticleLayer>();
    DAVA::ParticleEmitterInstance* emitter = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetDirectEmitterLayerOwner(layer);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandCloneParticleEmitterLayer(emitter, layer)));
}

void ParticleLayerContextMenu::RemoveLayer()
{
    auto fn = [this](const DAVA::Selectable& obj) -> std::unique_ptr<DAVA::Command> {
        DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(obj);
        DAVA::ParticleLayer* layer = obj.Cast<DAVA::ParticleLayer>();
        DAVA::ParticleEmitterInstance* emitter = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetDirectEmitterLayerOwner(layer);
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandRemoveParticleEmitterLayer(component, emitter, layer));
    };
    RemoveCommandsHelper("Remove layers", DAVA::ReflectedTypeDB::Get<DAVA::ParticleLayer>(), fn);
}

void ParticleLayerContextMenu::AddForce()
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticleEmitterSimplifiedForce(component, currentObject.Cast<DAVA::ParticleLayer>())));
}

void ParticleLayerContextMenu::AddDrag()
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticleDrag(component, currentObject.Cast<DAVA::ParticleLayer>())));
}

void ParticleLayerContextMenu::AddVortex()
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticleVortex(component, currentObject.Cast<DAVA::ParticleLayer>())));
}

void ParticleLayerContextMenu::AddGravity()
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticleGravity(component, currentObject.Cast<DAVA::ParticleLayer>())));
}

void ParticleLayerContextMenu::AddWind()
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticleWind(component, currentObject.Cast<DAVA::ParticleLayer>())));
}

void ParticleLayerContextMenu::AddPointGravity()
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticlePointGravity(component, currentObject.Cast<DAVA::ParticleLayer>())));
}

void ParticleLayerContextMenu::AddPlaneCollision()
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticlePlaneCollision(component, currentObject.Cast<DAVA::ParticleLayer>())));
}

ParticleSimplifiedForceContextMenu::ParticleSimplifiedForceContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject)
    : BaseContextMenu(scene, model, selectedObjects, currentObject)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ParticleSimplifiedForceContextMenu::FillActions(QMenu& menu)
{
    QString removeForce = selectedObjects.size() > 1 ? QStringLiteral("Remove Forces") : QStringLiteral("Remove Force");
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/remove_force.png"), removeForce), DAVA::MakeFunction(this, &ParticleSimplifiedForceContextMenu::RemoveForce));
}

void ParticleSimplifiedForceContextMenu::RemoveForce()
{
    auto fn = [this](const DAVA::Selectable& obj) {
        DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(obj);
        DAVA::ParticleForceSimplified* force = obj.Cast<DAVA::ParticleForceSimplified>();
        DAVA::ParticleLayer* layer = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetForceOwner(force);
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandRemoveParticleEmitterSimplifiedForce(component, layer, force));
    };

    RemoveCommandsHelper("Remove forces", DAVA::ReflectedTypeDB::Get<DAVA::ParticleForceSimplified>(), fn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ParticleForceContextMenu::ParticleForceContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject)
    : TBase(scene, model, selectedObjects, currentObject)
{
}

void ParticleForceContextMenu::FillActions(QMenu& menu)
{
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/clone.png"), QStringLiteral("Clone Force")), DAVA::MakeFunction(this, &ParticleForceContextMenu::CloneForce));
    QString removeForceHint;
    bool singleObject = selectedObjects.size() < 2;
    const QIcon* icon = nullptr;
    DVASSERT(currentObject.CanBeCastedTo<DAVA::ParticleForce>() == true);
    DAVA::ParticleForce* force = currentObject.Cast<DAVA::ParticleForce>();
    if (force->type == DAVA::ParticleForce::eType::DRAG_FORCE)
    {
        removeForceHint = (singleObject == true) ? QStringLiteral("Remove Drag Force") : QStringLiteral("Remove Drag Forces");
        icon = &DAVA::SharedIcon(":/QtIcons/remove_turtle.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::VORTEX)
    {
        removeForceHint = (singleObject == true) ? QStringLiteral("Remove Vortex") : QStringLiteral("Remove Vortices");
        icon = &DAVA::SharedIcon(":/QtIcons/vortex_ico_remove.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::GRAVITY)
    {
        removeForceHint = QStringLiteral("Remove Gravity");
        icon = &DAVA::SharedIcon(":/QtIcons/gravity_remove.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::WIND)
    {
        removeForceHint = QStringLiteral("Remove Wind");
        icon = &DAVA::SharedIcon(":/QtIcons/wind_p_remove.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::POINT_GRAVITY)
    {
        removeForceHint = QStringLiteral("Remove Point Gravity");
        icon = &DAVA::SharedIcon(":/QtIcons/pointGravity_remove.png");
    }
    else if (force->type == DAVA::ParticleForce::eType::PLANE_COLLISION)
    {
        removeForceHint = QStringLiteral("Remove Plane Collision");
        icon = &DAVA::SharedIcon(":/QtIcons/plane_coll_remove.png");
    }

    Connect(menu.addAction(*icon, removeForceHint), DAVA::MakeFunction(this, &ParticleForceContextMenu::RemoveForce));
}

void ParticleForceContextMenu::CloneForce()
{
    DVASSERT(currentObject.CanBeCastedTo<DAVA::ParticleForce>() == true);
    DAVA::ParticleForce* force = currentObject.Cast<DAVA::ParticleForce>();
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    DAVA::ParticleLayer* layer = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetForceOwner(force);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandCloneParticleForce(component, layer, force)));
}

void ParticleForceContextMenu::RemoveForce()
{
    DVASSERT(currentObject.CanBeCastedTo<DAVA::ParticleForce>() == true);
    DAVA::ParticleForce* force = currentObject.Cast<DAVA::ParticleForce>();
    DAVA::String commandName;
    if (force->type == DAVA::ParticleForce::eType::DRAG_FORCE)
        commandName = "Remove Drag force";
    else if (force->type == DAVA::ParticleForce::eType::VORTEX)
        commandName = "Remove Vortex";
    else if (force->type == DAVA::ParticleForce::eType::GRAVITY)
        commandName = "Remove Gravity";
    else if (force->type == DAVA::ParticleForce::eType::WIND)
        commandName = "Remove Wind";
    else if (force->type == DAVA::ParticleForce::eType::POINT_GRAVITY)
        commandName = "Remove Point Gravity";
    else if (force->type == DAVA::ParticleForce::eType::PLANE_COLLISION)
        commandName = "Remove Plane Collision";

    auto fn = [this](const DAVA::Selectable& obj) -> std::unique_ptr<DAVA::Command> {
        DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(obj);
        DAVA::ParticleForce* force = obj.Cast<DAVA::ParticleForce>();
        DAVA::ParticleLayer* layer = scene->GetSystem<DAVA::EditorParticlesSystem>()->GetForceOwner(force);
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandRemoveParticleForce(component, layer, force));
    };
    RemoveCommandsHelper(commandName, DAVA::ReflectedTypeDB::Get<DAVA::ParticleForce>(), fn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ParticleEmitterContextMenu::ParticleEmitterContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject)
    : BaseContextMenu(scene, model, selectedObjects, currentObject)
{
}

void ParticleEmitterContextMenu::FillActions(QMenu& menu)
{
    DAVA::ParticleEmitterInstance* instance = currentObject.Cast<DAVA::ParticleEmitterInstance>();

    if (instance->GetOwner() != nullptr)
    {
        QString removeEmitterText = selectedObjects.size() < 2 ? QStringLiteral("Remove emitter") : QStringLiteral("Remove emitters");
        Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/remove.png"), removeEmitterText), DAVA::MakeFunction(this, &ParticleEmitterContextMenu::RemoveEmitter));
        menu.addSeparator();
    }
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/layer_particle.png"), QStringLiteral("Add Layer")), DAVA::MakeFunction(this, &ParticleEmitterContextMenu::AddLayer));
    menu.addSeparator();
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/openscene.png"), QStringLiteral("Load Emitter from Yaml")), DAVA::MakeFunction(this, &ParticleEmitterContextMenu::LoadEmitterFromYaml));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/savescene.png"), QStringLiteral("Save Emitter to Yaml")), DAVA::MakeFunction(this, &ParticleEmitterContextMenu::SaveEmitterToYaml));
    Connect(menu.addAction(DAVA::SharedIcon(":/QtIcons/save_as.png"), QStringLiteral("Save Emitter to Yaml As...")), DAVA::MakeFunction(this, &ParticleEmitterContextMenu::SaveEmitterToYamlAs));
}

void ParticleEmitterContextMenu::RemoveEmitter()
{
    auto fn = [](const DAVA::Selectable& obj) {
        DAVA::ParticleEmitterInstance* emitterInstance = obj.Cast<DAVA::ParticleEmitterInstance>();
        DVASSERT(emitterInstance->GetOwner() != nullptr);
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandRemoveParticleEmitter(emitterInstance->GetOwner(), emitterInstance));
    };

    RemoveCommandsHelper("Remove Emitters", DAVA::ReflectedTypeDB::Get<DAVA::ParticleEmitterInstance>(), fn);
}

void ParticleEmitterContextMenu::AddLayer()
{
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::CommandAddParticleEmitterLayer(GetParticleEffectComponent(currentObject), currentObject.Cast<DAVA::ParticleEmitterInstance>())));
}

void ParticleEmitterContextMenu::LoadEmitterFromYaml()
{
    DAVA::CommonInternalSettings* settings = accessor->GetGlobalContext()->GetData<DAVA::CommonInternalSettings>();
    DAVA::FilePath defaultPath = settings->emitterLoadDir;
    QString particlesPath = defaultPath.IsEmpty() ? GetParticlesConfigPath() : QString::fromStdString(defaultPath.GetAbsolutePathname());

    DAVA::FileDialogParams params;
    params.dir = particlesPath;
    params.filters = QStringLiteral("YAML File (*.yaml)");
    params.title = QStringLiteral("Open Particle Emitter Yaml file");
    QString selectedPath = ui->GetOpenFileName(DAVA::mainWindowKey, params);

    if (selectedPath.isEmpty() == false)
    {
        DAVA::FilePath yamlPath = selectedPath.toStdString();
        settings->emitterLoadDir = yamlPath.GetDirectory();

        scene->Exec(CreateLoadCommand(yamlPath));
    }
}

void ParticleEmitterContextMenu::SaveEmitterToYaml()
{
    SaveEmitterLocal(false);
}

void ParticleEmitterContextMenu::SaveEmitterToYamlAs()
{
    SaveEmitterLocal(true);
}

void ParticleEmitterContextMenu::SaveEmitterLocal(bool forceAskFileName)
{
    SaveEmitter(GetParticleEffectComponent(currentObject), currentObject.Cast<DAVA::ParticleEmitterInstance>()->GetEmitter(),
                forceAskFileName, QString(),
                DAVA::MakeFunction(this, &ParticleEmitterContextMenu::CreateSaveCommand));
}

std::unique_ptr<DAVA::Command> ParticleEmitterContextMenu::CreateLoadCommand(const DAVA::FilePath& path)
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    DAVA::ParticleEmitterInstance* emitterInstance = currentObject.Cast<DAVA::ParticleEmitterInstance>();
    if (emitterInstance->GetOwner() != nullptr)
    {
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandLoadParticleEmitterFromYaml(component, emitterInstance, path));
    }
    else
    {
        QModelIndex layerIndex = model->GetIndexByObject(currentObject).parent();
        DAVA::Selectable layerObj = model->GetObjectByIndex(layerIndex);
        DVASSERT(layerObj.CanBeCastedTo<DAVA::ParticleLayer>());
        layerObj.Cast<DAVA::ParticleLayer>()->innerEmitterPath = path;
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandLoadInnerParticleEmitterFromYaml(emitterInstance, path));
    }
}

std::unique_ptr<DAVA::Command> ParticleEmitterContextMenu::CreateSaveCommand(const DAVA::FilePath& path)
{
    DAVA::ParticleEffectComponent* component = GetParticleEffectComponent(currentObject);
    DAVA::ParticleEmitterInstance* emitterInstance = currentObject.Cast<DAVA::ParticleEmitterInstance>();
    if (emitterInstance->GetOwner() != nullptr)
    {
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandSaveParticleEmitterToYaml(component, emitterInstance, path));
    }
    else
    {
        QModelIndex layerIndex = model->GetIndexByObject(currentObject).parent();
        DAVA::Selectable layerObj = model->GetObjectByIndex(layerIndex);
        DVASSERT(layerObj.CanBeCastedTo<DAVA::ParticleLayer>());
        layerObj.Cast<DAVA::ParticleLayer>()->innerEmitterPath = path;
        return std::unique_ptr<DAVA::Command>(new DAVA::CommandSaveInnerParticleEmitterToYaml(emitterInstance, path));
    }
}

std::unique_ptr<BaseContextMenu> CreateSceneTreeContextMenu(DAVA::SceneEditor2* scene, const SceneTreeModelV2* model, const DAVA::Vector<DAVA::Selectable>& selectedObjects, const DAVA::Selectable& currentObject)
{
    const DAVA::ReflectedType* objType = currentObject.GetObjectType();
    if (objType == DAVA::ReflectedTypeDB::Get<DAVA::Entity>())
    {
        return std::make_unique<EntityContextMenu>(scene, model, selectedObjects, currentObject);
    }

    if (objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleEmitterInstance>())
    {
        return std::make_unique<ParticleEmitterContextMenu>(scene, model, selectedObjects, currentObject);
    }

    if (objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleLayer>())
    {
        return std::make_unique<ParticleLayerContextMenu>(scene, model, selectedObjects, currentObject);
    }
    if (objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleForceSimplified>())
    {
        return std::make_unique<ParticleSimplifiedForceContextMenu>(scene, model, selectedObjects, currentObject);
    }
    if (objType == DAVA::ReflectedTypeDB::Get<DAVA::ParticleForce>())
    {
        return std::make_unique<ParticleForceContextMenu>(scene, model, selectedObjects, currentObject);
    }

    return nullptr;
}

IMPL_OPERATION_ID(SetSceneTreeFilter);
IMPL_OPERATION_ID(ReloadTexturesInSelectedOperation);
