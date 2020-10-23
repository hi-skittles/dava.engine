#include "Classes/Qt/Tools/AddSwitchEntityDialog/AddSwitchEntityDialog.h"
#include "Classes/Qt/Tools/AddSwitchEntityDialog/SwitchEntityCreator.h"
#include "Classes/Qt/Tools/SelectPathWidget/SelectEntityPathWidget.h"

#include <REPlatform/Commands/EntityAddCommand.h>
#include <REPlatform/Commands/EntityRemoveCommand.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SceneData.h>

#include "ui_BaseAddEntityDialog.h"

#include <QtTools/ConsoleWidget/PointerSerializer.h>

#include <TArc/Core/Deprecated.h>
#include <TArc/DataProcessing/DataContext.h>

#include <Scene3D/Components/MotionComponent.h>
#include <Scene3D/Components/SkeletonComponent.h>

#include <QPushButton>

namespace AddSwitchEntityDialogDetails
{
DAVA::SceneEditor2* GetActiveScene()
{
    DAVA::SceneData* data = DAVA::Deprecated::GetActiveDataNode<DAVA::SceneData>();
    if (data != nullptr)
    {
        return data->GetScene().Get();
    }
    return nullptr;
}
}

AddSwitchEntityDialog::AddSwitchEntityDialog(QWidget* parent)
    : BaseAddEntityDialog(parent, QDialogButtonBox::Ok | QDialogButtonBox::Cancel)
{
    setAcceptDrops(true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data != nullptr);
    DAVA::FilePath defaultPath(data->GetDataSource3DPath());

    DAVA::SceneEditor2* scene = AddSwitchEntityDialogDetails::GetActiveScene();
    if (scene != nullptr)
    {
        const DAVA::FilePath& scenePath = scene->GetScenePath();
        if (scenePath.Exists())
        {
            defaultPath = scenePath.GetDirectory();
        }
    }

    SelectEntityPathWidget* firstWidget = new SelectEntityPathWidget(parent, scene, defaultPath.GetAbsolutePathname(), "");
    SelectEntityPathWidget* secondWidget = new SelectEntityPathWidget(parent, scene, defaultPath.GetAbsolutePathname(), "");
    SelectEntityPathWidget* thirdWidget = new SelectEntityPathWidget(parent, scene, defaultPath.GetAbsolutePathname(), "");

    connect(firstWidget, &SelectEntityPathWidget::PathSelected, this, &AddSwitchEntityDialog::OnPathChanged);
    connect(secondWidget, &SelectEntityPathWidget::PathSelected, this, &AddSwitchEntityDialog::OnPathChanged);
    connect(thirdWidget, &SelectEntityPathWidget::PathSelected, this, &AddSwitchEntityDialog::OnPathChanged);

    AddControlToUserContainer(firstWidget, "First Entity:");
    AddControlToUserContainer(secondWidget, "Second Entity:");
    AddControlToUserContainer(thirdWidget, "Third Entity:");

    pathWidgets.push_back(firstWidget);
    pathWidgets.push_back(secondWidget);
    pathWidgets.push_back(thirdWidget);

    propEditor->setVisible(false);
    propEditor->setMinimumHeight(0);
    propEditor->setMaximumSize(propEditor->maximumWidth(), 0);

    OnPathChanged();
}

AddSwitchEntityDialog::~AddSwitchEntityDialog()
{
    RemoveAllControlsFromUserContainer();
}

void AddSwitchEntityDialog::OnPathChanged()
{
    QPushButton* okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    DVASSERT(okButton != nullptr);
    bool hasAtLeastOnePathSet = std::any_of(pathWidgets.begin(), pathWidgets.end(), [](SelectEntityPathWidget* w) { return w->getText().empty() == false; });
    okButton->setEnabled(hasAtLeastOnePathSet);
}

void AddSwitchEntityDialog::CleanupPathWidgets()
{
    for (SelectEntityPathWidget* widget : pathWidgets)
    {
        widget->EraseWidget();
    }
}

DAVA::Vector<DAVA::RefPtr<DAVA::Entity>> AddSwitchEntityDialog::GetPathEntities()
{
    DAVA::Vector<DAVA::RefPtr<DAVA::Entity>> entities;
    for (SelectEntityPathWidget* widget : pathWidgets)
    {
        DAVA::RefPtr<DAVA::Entity> entity = widget->GetOutputEntity();
        if (entity)
        {
            entities.push_back(entity);
        }
    }
    return entities;
}

void AddSwitchEntityDialog::accept()
{
    using namespace DAVA;

    SceneEditor2* scene = AddSwitchEntityDialogDetails::GetActiveScene();
    if (scene == nullptr)
    {
        CleanupPathWidgets();
        return;
    }

    Vector<RefPtr<Entity>> pathEntities = GetPathEntities();
    Vector<Entity*> sourceEntities(pathEntities.size());
    std::transform(pathEntities.begin(), pathEntities.end(), sourceEntities.begin(), [](const RefPtr<Entity>& r) { return r.Get(); });

    CleanupPathWidgets();

    SwitchEntityCreator creator;

    bool simpleSwitchMode = true; // simple switch mode is when each source entity contains exact one render object
    size_t simpleSkinnedMeshObjectsCount = 0;
    size_t simpleSkeletonComponentsCount = 0;
    size_t simpleMotionComponentsCount = 0;

    for (Entity* sourceEntity : sourceEntities)
    {
        if (creator.HasSwitchComponentsRecursive(sourceEntity))
        {
            Logger::Error("Can't create switch over switch: %s", sourceEntity->GetName().c_str());
            return;
        }

        size_t meshesCount = creator.GetRenderObjectsCountRecursive(sourceEntity, RenderObject::TYPE_MESH);
        size_t skinnedMeshesCount = creator.GetRenderObjectsCountRecursive(sourceEntity, RenderObject::TYPE_SKINNED_MESH);
        size_t overallMeshesCount = meshesCount + skinnedMeshesCount;

        if (overallMeshesCount == 0)
        {
            Logger::Error("Entity '%s' hasn't mesh / skinned mesh render objects", sourceEntity->GetName().c_str());
            return;
        }

        if (overallMeshesCount > 1)
        {
            simpleSwitchMode = false;
        }

        if (simpleSwitchMode)
        {
            if (skinnedMeshesCount > 0)
            {
                DVASSERT(skinnedMeshesCount == 1);
                ++simpleSkinnedMeshObjectsCount;
            }
            simpleSkeletonComponentsCount += sourceEntity->GetComponentCount<SkeletonComponent>();
            simpleMotionComponentsCount += sourceEntity->GetComponentCount<MotionComponent>();
        }
    }

    if (simpleSwitchMode)
    {
        if (simpleSkinnedMeshObjectsCount > 1)
        {
            Logger::Error("Can't create switch with multiple skinned mesh render objects");
            return;
        }
        else if (simpleSkeletonComponentsCount > 1)
        {
            Logger::Error("Can't create switch with skinned mesh and multiple skeleton components");
            return;
        }
        else if (simpleMotionComponentsCount > 1)
        {
            Logger::Error("Can't create switch with skinned mesh and multiple motion components");
            return;
        }
    }

    scene->BeginBatch("Unite entities into switch entity", static_cast<uint32>(sourceEntities.size()) + 1u);

    for (Entity* sourceEntity : sourceEntities)
    {
        scene->Exec(std::unique_ptr<Command>(new EntityRemoveCommand(sourceEntity)));
    }
    RefPtr<Entity> switchEntity = creator.CreateSwitchEntity(sourceEntities);
    scene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::EntityAddCommand(switchEntity.Get(), scene)));

    scene->EndBatch();

    BaseAddEntityDialog::accept();
}

void AddSwitchEntityDialog::reject()
{
    CleanupPathWidgets();
    BaseAddEntityDialog::reject();
}
