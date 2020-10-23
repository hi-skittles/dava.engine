#include "Classes/Application/REApplication.h"
#include "Classes/Application/LaunchModule.h"
#include "Classes/Application/ReflectionExtensions.h"
#include "Classes/Application/REModule.h"

#include "Classes/Project/ProjectManagerModule.h"
#include "Classes/SceneManager/SceneManagerModule.h"
#include "Classes/CommandLine/ConsoleHelpTool.h"
#include "Classes/CommandLine/DumpTool.h"
#include "Classes/CommandLine/SceneImageDump.h"
#include "Classes/CommandLine/StaticOcclusionTool.h"
#include "Classes/CommandLine/VersionTool.h"
#include "Classes/CommandLine/ImageSplitterTool.h"
#include "Classes/CommandLine/TextureDescriptorTool.h"
#include "Classes/CommandLine/SceneSaverTool.h"
#include "Classes/CommandLine/SceneExporterTool.h"
#include "Classes/CommandLine/SceneValidationTool.h"
#include "Classes/DevFuncs/TestUIModuleData.h"

#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <REPlatform/Deprecated/SceneValidator.h>
#include <REPlatform/Scene/Systems/VisibilityCheckSystem.h>

#include <QtTools/InitQtTools.h>

#include <TArc/Core/Core.h>
#include <TArc/Testing/TArcTestClass.h>
#include <TArc/DataProcessing/PropertiesHolder.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Utils/ReflectionHelpers.h>
#include <TArc/SharedModules/SettingsModule/SettingsModule.h>
#include <TArc/SharedModules/ThemesModule/ThemesModule.h>
#include <TArc/SharedModules/ActionManagementModule/ActionManagementModule.h>

#include <DocDirSetup/DocDirSetup.h>
#include <Version/Version.h>

#include <Base/BaseTypes.h>
#include <Core/PerformanceSettings.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/KeyedArchive.h>
#include <Particles/ParticleEmitter.h>
#include <Render/RHI/rhi_Type.h>
#include <Scene3D/Systems/QualitySettingsSystem.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>

namespace REApplicationDetail
{
rhi::Api Convert(DAVA::RenderingBackend r)
{
    switch (r)
    {
    case DAVA::RenderingBackend::DX11:
        return rhi::RHI_DX11;
    case DAVA::RenderingBackend::DX9:
        return rhi::RHI_DX9;
    case DAVA::RenderingBackend::OpenGL:
        return rhi::RHI_GLES2;
    default:
        DVASSERT(false);
        break;
    }

    return rhi::RHI_GLES2;
}
}

REApplication::REApplication(DAVA::Vector<DAVA::String>&& cmdLine_)
    : cmdLine(std::move(cmdLine_))
{
    if (cmdLine.size() > 1)
    {
        DAVA::String command = cmdLine[1];
        isConsoleMode = (command != "--selftest");
    }
}

DAVA::BaseApplication::EngineInitInfo REApplication::GetInitInfo() const
{
    EngineInitInfo initInfo;
    initInfo.runMode = isConsoleMode ? DAVA::eEngineRunMode::CONSOLE_MODE : DAVA::eEngineRunMode::GUI_EMBEDDED;
    initInfo.modules = DAVA::Vector<DAVA::String>
    {
      "JobManager",
      "NetCore",
      "LocalizationSystem",
      "SoundSystem",
      "DownloadManager",
    };

    initInfo.options.Set(CreateOptions());
    return initInfo;
}

void REApplication::CreateModules(DAVA::Core* tarcCore) const
{
    DAVA::ContextAccessor* accessor = tarcCore->GetCoreInterface();
    DAVA::PropertiesItem item = accessor->CreatePropertiesNode("renderBackendMirrorNode");
    DAVA::RenderingBackend renderBackend = item.Get("renderBackend", DAVA::RenderingBackend::OpenGL);

    appOptions->SetInt32("renderer", REApplicationDetail::Convert(renderBackend));

    renderBackEndListener.reset(new DAVA::FieldBinder(accessor));
    DAVA::FieldDescriptor descr;
    descr.type = DAVA::ReflectedTypeDB::Get<DAVA::GeneralSettings>();
    descr.fieldName = DAVA::FastName("renderBackend");

    renderBackEndListener->BindField(descr, [accessor](const DAVA::Any& v) {
        if (v.IsEmpty() == true)
        {
            return;
        }

        DAVA::PropertiesItem item = accessor->CreatePropertiesNode("renderBackendMirrorNode");
        item.Set("renderBackend", v);
    });

    if (isConsoleMode)
    {
        CreateConsoleModules(tarcCore);
    }
    else
    {
        CreateGUIModules(tarcCore);
    }
}

void REApplication::Init(const DAVA::EngineContext* engineContext)
{
    using namespace DAVA;
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(GeneralSettings);
    DAVA_REFLECTION_REGISTER_PERMANENT_NAME(CommonInternalSettings);

    DAVA::ParticleEmitter::FORCE_DEEP_CLONE = true;
    DAVA::QualitySettingsSystem::Instance()->SetKeepUnusedEntities(true);
    DAVA::QualitySettingsSystem::Instance()->SetRuntimeQualitySwitching(true);

    DAVA::FileSystem* fileSystem = engineContext->fileSystem;
    
#ifdef __DAVAENGINE_MACOS__
    auto copyDocumentsFromOldFolder = [&]
    {
        DAVA::FileSystem::eCreateDirectoryResult createResult = DAVA::DocumentsDirectorySetup::CreateApplicationDocDirectory(fileSystem, "ResourceEditor");
        if (createResult != DAVA::FileSystem::DIRECTORY_EXISTS)
        {
            DAVA::FilePath documentsOldFolder = fileSystem->GetCurrentDocumentsDirectory() + "ResourceEditor/";
            DAVA::FilePath documentsNewFolder = DAVA::DocumentsDirectorySetup::GetApplicationDocDirectory(fileSystem, "ResourceEditor");
            fileSystem->RecursiveCopy(documentsOldFolder, documentsNewFolder);
        }
    };
    copyDocumentsFromOldFolder(); // todo: remove function some versions after
#endif

    DAVA::DocumentsDirectorySetup::SetApplicationDocDirectory(fileSystem, "ResourceEditor");

    engineContext->logger->SetLogFilename("ResourceEditor.txt");
    engineContext->logger->Log(DAVA::Logger::LEVEL_INFO, QString("Qt version: %1").arg(QT_VERSION_STR).toStdString().c_str());
    engineContext->logger->Log(DAVA::Logger::LEVEL_INFO, DAVA::Version::CreateAppVersion("App Version: Resource Editor").c_str());
    engineContext->uiControlSystem->vcs->EnableReloadResourceOnResize(false);
    engineContext->performanceSettings->SetPsPerformanceMinFPS(5.0f);
    engineContext->performanceSettings->SetPsPerformanceMaxFPS(10.0f);

    BaseApplication::Init(engineContext);
}

void REApplication::Init(DAVA::Core* tarcCore)
{
    tarcCore->InitPluginManager("ResourceEditor", DAVA::GetEngineContext()->fileSystem->GetPluginDirectory().GetAbsolutePathname());
    BaseApplication::Init(tarcCore);
}

void REApplication::Cleanup()
{
    DAVA::VisibilityCheckSystem::ReleaseCubemapRenderTargets();

    cmdLine.clear();
}

bool REApplication::AllowMultipleInstances() const
{
    bool isSelfTest = (cmdLine.size() > 1) && (cmdLine[1] == "--selftest");
    return isSelfTest || isConsoleMode;
}

QString REApplication::GetInstanceKey() const
{
    DAVA::String appPath = cmdLine.front();
    QFileInfo appFileInfo(QString::fromStdString(appPath));

    const QString appUid = "{AA5497E4-6CE2-459A-B26F-79AAF05E0C6B}";
    const QString appUidPath = QCryptographicHash::hash((appUid + appFileInfo.absoluteDir().absolutePath()).toUtf8(), QCryptographicHash::Sha1).toHex();
    return appUidPath;
}

void REApplication::CreateGUIModules(DAVA::Core* tarcCore) const
{
    using namespace DAVA;
    InitColorPickerOptions(false);
    InitQtTools();

    tarcCore->CreateModule<DAVA::SettingsModule>();
    tarcCore->CreateModule<DAVA::ThemesModule>();
    tarcCore->CreateModule<DAVA::ActionManagementModule>();
    tarcCore->CreateModule<ReflectionExtensionsModule>();
    tarcCore->CreateModule<REModule>();
    tarcCore->CreateModule<ProjectManagerModule>();
    tarcCore->CreateModule<SceneManagerModule>();

    for (const DAVA::ReflectedType* type : DAVA::ModuleCollection::Instance()->GetGuiModules())
    {
        tarcCore->CreateModule(type);
    }

    tarcCore->CreateModule<LaunchModule>();
}

void REApplication::CreateConsoleModules(DAVA::Core* tarcCore) const
{
    DVASSERT(cmdLine.size() > 1);

    DAVA::Vector<const DAVA::ReflectedType*> modules = DAVA::ModuleCollection::Instance()->GetConsoleModules();

    auto createModuleFn = [&](const DAVA::String& command) -> bool
    {
        for (const DAVA::ReflectedType* moduleType : modules)
        {
            const DAVA::M::CommandName* commandNameMeta = DAVA::GetReflectedTypeMeta<DAVA::M::CommandName>(moduleType);
            if (commandNameMeta != nullptr && commandNameMeta->commandName == command)
            {
                tarcCore->CreateModule(moduleType, cmdLine);
                return true;
            }
        }

        return false;
    };

    DAVA::String command = cmdLine[1];
    if (createModuleFn(command) == false)
    {
        DAVA::Logger::Error("Cannot create commandLine module for command \'%s\'", command.c_str());
        createModuleFn("-help");
    }
}

void REApplication::RegisterEditorAnyCasts()
{
    DAVA::BaseApplication::RegisterEditorAnyCasts();

    DAVA::AnyCast<ComboBoxTestDataDescr, DAVA::String>::Register(&ComboBoxTestDataDescrToString);
    DAVA::AnyCast<ComboBoxTestDataDescr, QIcon>::Register(&ComboBoxTestDataDescrToQIcon);
}

DAVA::KeyedArchive* REApplication::CreateOptions() const
{
    appOptions.ConstructInplace();

    appOptions->SetInt32("bpp", 32);
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
    appOptions->SetInt32("max_index_buffer_count", 16384);
    appOptions->SetInt32("max_vertex_buffer_count", 16384);
    appOptions->SetInt32("max_const_buffer_count", 32767);
    appOptions->SetInt32("max_texture_count", 2048);

    appOptions->SetInt32("max_pipeline_state_count", 32 * 1024);

    appOptions->SetInt32("shader_const_buffer_size", 256 * 1024 * 1024);

    appOptions->SetBool("separate_net_thread", true);

    appOptions->Retain();

    return appOptions.Get();
}
