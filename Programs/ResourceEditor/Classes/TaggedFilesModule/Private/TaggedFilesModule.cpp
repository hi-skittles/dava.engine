#include "Classes/TaggedFilesModule/TaggedFilesModule.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <REPlatform/Scene/SceneHelper.h>

#include <TextureCompression/TextureConverter.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Qt/QtString.h>

#include <Engine/Engine.h>
#include <FileSystem/FileSystem.h>
#include <Render/Texture.h>
#include <Scene3D/Scene.h>
#include "REPlatform/Global/GlobalOperations.h"

namespace TaggedFilesModuleDetails
{
DAVA::Vector<DAVA::FilePath> CollectTaggedTextures(DAVA::ContextAccessor* accessor, DAVA::Scene* scene)
{
    using namespace DAVA;
    DVASSERT(scene != nullptr);

    Vector<FilePath> taggedTextures;

    ProjectManagerData* data = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
    const EditorConfig* editorConfig = data->GetEditorConfig();
    if (editorConfig->HasProperty("Tags") == true)
    {
        FileSystem* fs = GetEngineContext()->fileSystem;

        SceneHelper::TextureCollector collector;
        SceneHelper::EnumerateSceneTextures(scene, collector);
        TexturesMap& collectedTextures = collector.GetTextures();

        const Vector<String>& tags = editorConfig->GetComboPropertyValues("Tags");
        taggedTextures.reserve(tags.size() * collectedTextures.size());
        for (const std::pair<FilePath, Texture*>& pair : collectedTextures)
        { // find textures with tags only
            for (const String& tag : tags)
            {
                FilePath path = pair.first;
                path.ReplaceBasename(path.GetBasename() + tag);
                if (fs->Exists(path) == true)
                {
                    taggedTextures.push_back(path);
                }
            }
        }
    }

    return taggedTextures;
}
}

void TaggedFilesModule::PostInit()
{
    using namespace DAVA;
    using namespace DAVA;

    QtAction* convertTaggedAction = new QtAction(GetAccessor(), QIcon(":/QtIconsTextureDialog/convertTagged.png"), "Convert tagged textures");

    FieldDescriptor fieldDescriptor;
    fieldDescriptor.type = ReflectedTypeDB::Get<SceneData>();
    fieldDescriptor.fieldName = FastName(SceneData::scenePropertyName);
    convertTaggedAction->SetStateUpdationFunction(QtAction::Enabled, fieldDescriptor, [](const Any& value) -> Any
                                                  {
                                                      return value.CanCast<SceneData::TSceneType>();
                                                  });

    ActionPlacementInfo menuPlacement(CreateMenuPoint("Scene", InsertionParams(InsertionParams::eInsertionMethod::AfterItem, QStringLiteral("actionConvertModifiedTextures"))));
    GetUI()->AddAction(DAVA::mainWindowKey, menuPlacement, convertTaggedAction);

    connections.AddConnection(convertTaggedAction, &QAction::triggered, DAVA::MakeFunction(this, &TaggedFilesModule::OnConvertTaggedTextures));
    RegisterOperation(DAVA::ConvertTaggedTextures.ID, this, &TaggedFilesModule::OnConvertTaggedTextures);
}

void TaggedFilesModule::OnConvertTaggedTextures()
{
    using namespace DAVA;

    ContextAccessor* accessor = GetAccessor();
    SceneData* sceneData = accessor->GetActiveContext()->GetData<SceneData>();
    if (sceneData == nullptr)
    {
        return;
    }

    RefPtr<SceneEditor2> scene = sceneData->GetScene();
    DVASSERT(scene);

    WaitDialogParams params;
    params.needProgressBar = true;
    params.message = "Conversion of modified tagged textures.";
    params.min = 0;
    params.max = 1;
    std::unique_ptr<WaitHandle> waitHandle = GetUI()->ShowWaitDialog(DAVA::mainWindowKey, params);

    Vector<FilePath> taggedTextures = TaggedFilesModuleDetails::CollectTaggedTextures(accessor, scene.Get());
    if (taggedTextures.empty() == false)
    {
        DAVA::TextureConverter::eConvertQuality quality = accessor->GetGlobalContext()->GetData<GeneralSettings>()->compressionQuality;

        uint32 count = static_cast<uint32>(taggedTextures.size());
        waitHandle->SetRange(0, count * eGPUFamily::GPU_DEVICE_COUNT);

        uint32 index = 0;
        for (const FilePath& texPath : taggedTextures)
        {
            QString pathString = texPath.GetAbsolutePathname().c_str();
            for (int32 g = 0; g < eGPUFamily::GPU_DEVICE_COUNT; ++g)
            {
                eGPUFamily gpu = static_cast<eGPUFamily>(g);

                QString gpuName = QString::fromStdString(GPUFamilyDescriptor::GetGPUName(gpu));
                waitHandle->SetMessage(gpuName + ": " + pathString);
                waitHandle->SetProgressValue(index++);

                std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPath));
                if (descriptor)
                {
                    if (GPUFamilyDescriptor::IsFormatSupported(gpu, static_cast<PixelFormat>(descriptor->compression[gpu].format))
                        && (descriptor->IsCompressedTextureActual(gpu) == false))
                    { // we can convert only not actual textures
                        TextureConverter::ConvertTexture(*descriptor, gpu, true, quality);
                    }
                }
            }
        }
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(TaggedFilesModule)
{
    DAVA::ReflectionRegistrator<TaggedFilesModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_TARC_MODULE(TaggedFilesModule);
