#include "Classes/Project/ProjectManagerModule.h"
#include "Classes/Application/FileSystemData.h"
#include "Classes/Qt/MaterialEditor/MaterialEditor.h"
#include "Classes/Qt/TextureBrowser/TextureBrowser.h"

#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/ProjectResources.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/DataNodes/SpritesPackerModule.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <REPlatform/Global/Constants.h>

#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/QtAction.h>

#include <Scene3D/Systems/QualitySettingsSystem.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectedObject.h>
#include <Engine/EngineContext.h>
#include <FileSystem/YamlParser.h>
#include <FileSystem/YamlNode.h>
#include <FileSystem/KeyedArchive.h>
#include <FileSystem/FileSystem.h>
#include <Base/Result.h>
#include "Reflection/ReflectedTypeDB.h"
#include "Base/FastName.h"
#include "REPlatform/Global/GlobalOperations.h"
#include "Debug/DVAssert.h"

namespace ProjectManagerDetails
{
const DAVA::String PROPERTIES_KEY = "ProjectManagerProperties";
const DAVA::String LAST_PROJECT_PATH_KEY = "Internal/LastProjectPath";
}

ProjectManagerModule::ProjectManagerModule()
{
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(ProjectManagerModule);
}

ProjectManagerModule::~ProjectManagerModule() = default;

void ProjectManagerModule::PostInit()
{
    using namespace DAVA;

    projectResources.reset(new ProjectResources(GetAccessor()));

    DataContext* globalContext = GetAccessor()->GetGlobalContext();
    DAVA::ProjectManagerData* data = globalContext->GetData<DAVA::ProjectManagerData>();
    data->spritesPacker.reset(new SpritesPackerModule(GetUI(), GetAccessor()));
    data->editorConfig.reset(new EditorConfig());

    CreateActions();
    RegisterOperations();

    RecentMenuItems::Params params(DAVA::mainWindowKey, GetAccessor(), "Recent projects");
    params.ui = GetUI();
    params.getMaximumCount = []() {
        return 5;
    };

    params.recentMenuName = "Recent Projects";
    params.recentMenuPlacementInfo.AddPlacementPoint(CreateMenuPoint(MenuItems::menuFile, InsertionParams(InsertionParams::eInsertionMethod::AfterItem, "Open Project")));

    params.menuSubPath << MenuItems::menuFile << params.recentMenuName;

    recentProjects.reset(new RecentMenuItems(std::move(params)));
    recentProjects->actionTriggered.Connect([this](const DAVA::String& projectPath)
                                            {
                                                OpenProjectByPath(DAVA::FilePath(projectPath));
                                            });
}

void ProjectManagerModule::CreateActions()
{
    using namespace DAVA;
    UI* ui = GetUI();

    const QString openProjectName("Open Project");
    const QString recentProjectsName("Recent Projects");
    const QString closeProjectsName("Close Project");

    // OpenProject action
    {
        QAction* openProjectAction = new QAction(QIcon(":/QtIcons/openproject.png"), openProjectName, nullptr);
        connections.AddConnection(openProjectAction, &QAction::triggered, [this]()
                                  {
                                      OpenProject();
                                  });

        ActionPlacementInfo placementInfo(CreateMenuPoint(MenuItems::menuFile, InsertionParams(InsertionParams::eInsertionMethod::BeforeItem)));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, openProjectAction);
    }

    { // Close project
        QtAction* closeProjectAction = new QtAction(GetAccessor(), closeProjectsName, nullptr);

        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<DAVA::ProjectManagerData>();
        fieldDescr.fieldName = FastName(DAVA::ProjectManagerData::ProjectPathProperty);
        closeProjectAction->SetStateUpdationFunction(QtAction::Enabled, fieldDescr, [](const DAVA::Any& fieldValue) -> DAVA::Any {
            return fieldValue.CanCast<DAVA::FilePath>() && !fieldValue.Cast<DAVA::FilePath>().IsEmpty();
        });
        connections.AddConnection(closeProjectAction, &QAction::triggered, [this]() {
            CloseProject();
        });
        ActionPlacementInfo placementInfo(DAVA::CreateMenuPoint(MenuItems::menuFile, InsertionParams(InsertionParams::eInsertionMethod::AfterItem, openProjectName)));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, closeProjectAction);
    }

    // Separator
    {
        QAction* separator = new QAction(nullptr);
        separator->setObjectName(QStringLiteral("projectSeparator"));
        separator->setSeparator(true);
        ActionPlacementInfo placementInfo(CreateMenuPoint(MenuItems::menuFile, InsertionParams(InsertionParams::eInsertionMethod::AfterItem, closeProjectsName)));
        ui->AddAction(DAVA::mainWindowKey, placementInfo, separator);
    }

    InsertionParams reloadSpritesInsertionParams;
    reloadSpritesInsertionParams.method = InsertionParams::eInsertionMethod::BeforeItem;
    ActionPlacementInfo reloadSpritePlacement(CreateToolbarPoint("sceneToolBar", reloadSpritesInsertionParams));
    QAction* reloadSprites = new QAction(QIcon(":/QtIcons/refresh_particle.png"), "Reload Sprites", nullptr);
    connections.AddConnection(reloadSprites, &QAction::triggered, [this]()
                              {
                                  ReloadSprites();
                              });
    ui->AddAction(mainWindowKey, reloadSpritePlacement, reloadSprites);
}

void ProjectManagerModule::CreateTagsActions()
{
    using namespace DAVA;

    FieldDescriptor projFieldDescriptor;
    projFieldDescriptor.type = ReflectedTypeDB::Get<ProjectManagerData>();
    projFieldDescriptor.fieldName = FastName(ProjectManagerData::ProjectPathProperty);

    FieldDescriptor tagFieldDescr;
    tagFieldDescr.type = ReflectedTypeDB::Get<FileSystemData>();
    tagFieldDescr.fieldName = FastName("tag");

    //create menu item
    QtAction* actionTags = new QtAction(GetAccessor(), "Tags");
    Reflection model = Reflection::Create(ReflectedObject(this));
    actionTags->SetStateUpdationFunction(QtAction::Enabled, model, FastName("tagsEnabled"), [](const Any& fieldValue) -> Any {
        return fieldValue.Get<bool>(false);
    });
    ActionPlacementInfo placementTags(CreateMenuPoint(MenuItems::menuView, InsertionParams(InsertionParams::eInsertionMethod::BeforeItem, "GPU")));
    GetUI()->AddAction(DAVA::mainWindowKey, placementTags, actionTags);

    // create tag-specific actions
    QActionGroup* actionGroup = new QActionGroup(actionTags);
    actionGroup->setExclusive(true);

    ActionPlacementInfo placement(CreateMenuPoint(QList<QString>() << MenuItems::menuView << "Tags"));

    auto createTagAction = [&](const String& tagName, const String& tag)
    {
        QtAction* action = new QtAction(GetAccessor(), QString::fromStdString(tagName), nullptr);
        actionGroup->addAction(action);

        action->SetStateUpdationFunction(QtAction::Enabled, model, FastName("tagsEnabled"), [](const Any& fieldValue) -> Any {
            return fieldValue.Get<bool>(false);
        });

        action->SetStateUpdationFunction(QtAction::Checked, tagFieldDescr, [tag](const Any& v)
                                         {
                                             return v.Cast<DAVA::String>("") == tag;
                                         });
        connections.AddConnection(action, &QAction::triggered, DAVA::Bind(&ProjectManagerModule::SetFilenamesTag, this, tag), Qt::QueuedConnection);

        GetUI()->AddAction(DAVA::mainWindowKey, placement, action);
    };

    createTagAction("No tags", "");

    ProjectManagerData* data = GetAccessor()->GetGlobalContext()->GetData<ProjectManagerData>();
    if (data->editorConfig->HasProperty("Tags"))
    {
        const Vector<String>& projectTags = data->editorConfig->GetComboPropertyValues("Tags");
        for (const String& tag : projectTags)
        {
            createTagAction(tag, tag);
        }
    }
}

void ProjectManagerModule::RemoveTagsActions()
{
    using namespace DAVA;
    ActionPlacementInfo placementTags(CreateMenuPoint(QList<QString>() << MenuItems::menuView));
    GetUI()->RemoveAction(DAVA::mainWindowKey, placementTags, "Tags");
}

void ProjectManagerModule::RegisterOperations()
{
    RegisterOperation(DAVA::OpenLastProjectOperation.ID, this, &ProjectManagerModule::OpenLastProject);
}

void ProjectManagerModule::OpenProject()
{
    DAVA::DirectoryDialogParams dirDialogParams;
    dirDialogParams.title = QString("Open Project Folder");
    QString dirPath = GetUI()->GetExistingDirectory(DAVA::mainWindowKey, dirDialogParams);
    if (!dirPath.isEmpty())
    {
        DAVA::FilePath path(dirPath.toStdString());
        path.MakeDirectoryPathname();
        OpenProjectByPath(path);
    }
}

void ProjectManagerModule::OpenProjectByPath(const DAVA::FilePath& incomePath)
{
    DAVA::ProjectManagerData* data = GetData();

    if (incomePath.IsDirectoryPathname() && incomePath != data->projectPath)
    {
        bool closed = CloseProject();
        if (closed == false)
        {
            return;
        }

        DAVA::ContextAccessor* accessor = GetAccessor();
        DAVA::FileSystem* fileSystem = accessor->GetEngineContext()->fileSystem;
        if (fileSystem->Exists(incomePath))
        {
            DAVA::DataContext* ctx = accessor->GetGlobalContext();
            bool reloadParticles = ctx->GetData<DAVA::GeneralSettings>()->reloadParticlesOnProjectOpening;
            DVASSERT(data->spritesPacker != nullptr);
            if (reloadParticles)
            {
                auto func = DAVA::Bind(&ProjectManagerModule::OpenProjectImpl, this, incomePath);
                connections.AddConnection(data->spritesPacker.get(), &DAVA::SpritesPackerModule::SpritesReloaded, func, Qt::QueuedConnection);
                data->spritesPacker->RepackImmediately(incomePath, ctx->GetData<DAVA::CommonInternalSettings>()->spritesViewGPU);
            }
            else
            {
                OpenProjectImpl(incomePath);
            }
        }
    }
}

void ProjectManagerModule::OpenProjectImpl(const DAVA::FilePath& incomePath)
{
    DAVA::ProjectManagerData* data = GetData();
    connections.RemoveConnection(data->spritesPacker.get(), &DAVA::SpritesPackerModule::SpritesReloaded);

    const DAVA::EngineContext* engineCtx = GetAccessor()->GetEngineContext();
    DAVA::FileSystem* fileSystem = engineCtx->fileSystem;

    projectResources->LoadProject(incomePath);
    DAVA::PropertiesItem propsItem = GetAccessor()->CreatePropertiesNode(ProjectManagerDetails::PROPERTIES_KEY);

    DAVA::FilePath editorConfigPath = data->GetProjectPath() + "EditorConfig.yaml";
    if (fileSystem->Exists(editorConfigPath))
    {
        data->editorConfig->ParseConfig(editorConfigPath);
    }
    else
    {
        DAVA::Logger::Warning("Selected project doesn't contains EditorConfig.yaml");
    }

    propsItem.Set(ProjectManagerDetails::LAST_PROJECT_PATH_KEY, DAVA::Any(data->projectPath));

    recentProjects->Add(incomePath.GetAbsolutePathname());

    CreateTagsActions();
}

void ProjectManagerModule::OpenLastProject()
{
    DAVA::FilePath projectPath;
    {
        DAVA::PropertiesItem propsItem = GetAccessor()->CreatePropertiesNode(ProjectManagerDetails::PROPERTIES_KEY);
        projectPath = propsItem.Get<DAVA::FilePath>(ProjectManagerDetails::LAST_PROJECT_PATH_KEY);
    }

    if (!projectPath.IsEmpty())
    {
        DVASSERT(projectPath.IsDirectoryPathname());
        OpenProjectByPath(projectPath);
    }
}

bool ProjectManagerModule::CloseProject()
{
    DAVA::ProjectManagerData* data = GetData();
    if (!data->projectPath.IsEmpty())
    {
        InvokeOperation(DAVA::CloseAllScenesOperation.ID, true);
        if (GetAccessor()->GetContextCount() != 0)
        {
            return false;
        }

        projectResources->UnloadProject();

        DAVA::PropertiesItem propsItem = GetAccessor()->CreatePropertiesNode(ProjectManagerDetails::PROPERTIES_KEY);
        propsItem.Set(ProjectManagerDetails::LAST_PROJECT_PATH_KEY, DAVA::Any(DAVA::FilePath()));

        DAVA::CommonInternalSettings* settings = GetAccessor()->GetGlobalContext()->GetData<DAVA::CommonInternalSettings>();
        settings->emitterSaveDir = DAVA::FilePath();
        settings->emitterLoadDir = DAVA::FilePath();

        { // tags
            DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
            fs->SetFilenamesTag("");
            RemoveTagsActions();
        }
    }

    return true;
}

void ProjectManagerModule::ReloadSprites()
{
    using namespace DAVA;

    DataContext* ctx = GetAccessor()->GetGlobalContext();
    ProjectManagerData* data = ctx->GetData<ProjectManagerData>();
    DVASSERT(data);
    data->spritesPacker->RepackWithDialog();
}

DAVA::ProjectManagerData* ProjectManagerModule::GetData()
{
    using namespace DAVA;
    ContextAccessor* accessor = GetAccessor();
    DataContext* ctx = accessor->GetGlobalContext();
    return ctx->GetData<ProjectManagerData>();
}

void ProjectManagerModule::SetFilenamesTag(const DAVA::String& tag)
{
    DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
    if (fs->GetFilenamesTag() != tag)
    {
        fs->SetFilenamesTag(tag);
        if (GetAccessor()->GetActiveContext() != nullptr)
        { // reload textures only if we have opened scenes
            delayedExecutor.DelayedExecute([this]() {
                InvokeOperation(DAVA::ReloadAllTextures.ID, GetAccessor()->GetGlobalContext()->GetData<DAVA::CommonInternalSettings>()->textureViewGPU);
            });

            DAVA::NotificationParams notifParams;
            notifParams.title = "Information";
            notifParams.message.message = "Textures were reloaded. Reopen scene if you need to see other tagged resources.";
            GetUI()->ShowNotification(DAVA::mainWindowKey, notifParams);
        }
    }
}

bool ProjectManagerModule::GetTagsEnabled()
{
    DAVA::ProjectManagerData* projectData = GetData();
    if (projectData != nullptr && projectData->GetProjectPath().IsEmpty() == false)
    {
        DAVA::DataContext* activeContext = GetAccessor()->GetActiveContext();
        if (activeContext != nullptr)
        {
            MaterialEditor* editor = MaterialEditor::Instance();
            TextureBrowser* browser = TextureBrowser::Instance();
            if (editor == nullptr && browser == nullptr)
            {
                return true;
            }

            bool enableToChangeTags = (editor->isVisible() == false) && (browser->isVisible() == false);

            DAVA::SceneData* sceneData = activeContext->GetData<DAVA::SceneData>();
            DAVA::RefPtr<DAVA::SceneEditor2> scene = sceneData->GetScene();

            return (enableToChangeTags && (scene->GetEnabledTools() == false));
        }
        return true;
    }
    return false;
}
