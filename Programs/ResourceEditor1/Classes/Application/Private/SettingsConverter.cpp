#include "Classes/Application/Private/SettingsConverter.h"
#include "Classes/Application/RESettings.h"
#include "Classes/SceneManager/SceneData.h"

#include <TArc/DataProcessing/PropertiesHolder.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Controls/ColorPicker/ColorPickerSettings.h>
#include <TArc/SharedModules/ThemesModule/ThemesModule.h>

#include <Engine/PlatformApiQt.h>
#include <FileSystem/KeyedArchive.h>
#include <Render/RenderBase.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Base/BaseTypes.h>

namespace SettingsConverterDetail
{
const DAVA::String versionPropertyKey = "settingsVersion";
const DAVA::uint32 INIT_SETTINGS_VERSION = 1;
const DAVA::uint32 CURRENT_VERSION = INIT_SETTINGS_VERSION;

class OldSettingsConverter
{
public:
    OldSettingsConverter()
    {
        {
            DAVA::KeyedArchive* toLoad = new DAVA::KeyedArchive();
            if (toLoad->Load(settingsFilePath))
            {
                const DAVA::KeyedArchive::UnderlyingMap& values = toLoad->GetArchieveData();
                for (const auto& valueNode : values)
                {
                    settingsMap.emplace(DAVA::FastName(valueNode.first), CustomTextureViewGPULoad(valueNode.first, *valueNode.second));
                }
            }

            SafeRelease(toLoad);
        }

        {
            DAVA::KeyedArchive* toLoad = new DAVA::KeyedArchive();
            if (toLoad->Load(settingsFilePath2))
            {
                std::function<void(DAVA::KeyedArchive*)> unpackKeyedArchive = [&](DAVA::KeyedArchive* archive)
                {
                    const DAVA::KeyedArchive::UnderlyingMap& values = archive->GetArchieveData();
                    for (const auto& valueNode : values)
                    {
                        if (valueNode.second->GetType() == DAVA::VariantType::TYPE_KEYED_ARCHIVE)
                        {
                            DAVA::KeyedArchive* archive = valueNode.second->AsKeyedArchive();
                            unpackKeyedArchive(archive);
                        }
                        else
                        {
                            settingsMap2[DAVA::FastName(valueNode.first)] = *valueNode.second;
                        }
                    }
                };

                unpackKeyedArchive(toLoad);
            }

            SafeRelease(toLoad);
        }
    }

    void Do(const DAVA::TArc::PropertiesHolder& rootSettingsNode, DAVA::TArc::ContextAccessor* accessor) const
    {
        using namespace DAVA;
        using namespace DAVA::TArc;

        DataContext* ctx = accessor->GetGlobalContext();
        GeneralSettings* generalSettings = ctx->GetData<GeneralSettings>();
        DVASSERT(generalSettings != nullptr);

        GlobalSceneSettings* globalSceneSettings = ctx->GetData<GlobalSceneSettings>();
        DVASSERT(globalSceneSettings);

        CommonInternalSettings* internalSettings = ctx->GetData<CommonInternalSettings>();
        DVASSERT(internalSettings);

        DAVA::TArc::ColorPickerSettings* colorPickerSettings = ctx->GetData<DAVA::TArc::ColorPickerSettings>();
        DVASSERT(colorPickerSettings);

        DAVA::TArc::ThemesSettings* themeSettings = ctx->GetData<DAVA::TArc::ThemesSettings>();
        DVASSERT(themeSettings);

#define LOAD_SETTING(settingsVar, field, key, convertFn)\
        settingsVar->field = GetValue(key, settingsVar->field).convertFn()

        LOAD_SETTING(generalSettings, reloadParticlesOnProjectOpening, General_ReloadParticlesOnPojectOpening, AsBool);
        LOAD_SETTING(generalSettings, previewEnabled, General_PreviewEnabled, AsBool);

        generalSettings->compressionQuality = GetEnumValue(General_CompressionQuality, generalSettings->compressionQuality);
        LOAD_SETTING(generalSettings, showErrorDialog, General_ShowErrorDialog, AsBool);
        LOAD_SETTING(generalSettings, materialEditorSwitchColor0, General_MaterialEditor_SwitchColor0, AsColor);
        LOAD_SETTING(generalSettings, materialEditorSwitchColor1, General_MaterialEditor_SwitchColor1, AsColor);

        LOAD_SETTING(generalSettings, materialEditorLodColor0, General_MaterialEditor_LodColor0, AsColor);
        LOAD_SETTING(generalSettings, materialEditorLodColor1, General_MaterialEditor_LodColor1, AsColor);
        LOAD_SETTING(generalSettings, materialEditorLodColor2, General_MaterialEditor_LodColor2, AsColor);
        LOAD_SETTING(generalSettings, materialEditorLodColor3, General_MaterialEditor_LodColor3, AsColor);

        LOAD_SETTING(generalSettings, lodEditorLodColor0, General_LODEditor_LodColor0, AsColor);
        LOAD_SETTING(generalSettings, lodEditorLodColor1, General_LODEditor_LodColor1, AsColor);
        LOAD_SETTING(generalSettings, lodEditorLodColor2, General_LODEditor_LodColor2, AsColor);
        LOAD_SETTING(generalSettings, lodEditorLodColor3, General_LODEditor_LodColor3, AsColor);
        LOAD_SETTING(generalSettings, inactiveColor, General_LODEditor_InactiveColor, AsColor);
        LOAD_SETTING(generalSettings, fitSliders, General_LODEditor_FitSliders, AsBool);

        LOAD_SETTING(generalSettings, heightMaskColor0, General_HeighMaskTool_Color0, AsColor);
        LOAD_SETTING(generalSettings, heightMaskColor1, General_HeighMaskTool_Color1, AsColor);

        LOAD_SETTING(generalSettings, particleDebugAlphaTheshold, General_ParticleEditor_ParticleDebugAlphaTheshold, AsFloat);

        LOAD_SETTING(generalSettings, useAssetCache, General_AssetCache_UseCache, AsBool);
        LOAD_SETTING(generalSettings, assetCacheIP, General_AssetCache_Ip, AsString);
        generalSettings->assetCachePort = GetValue(General_AssetCache_Port, static_cast<DAVA::uint32>(generalSettings->assetCachePort)).AsUInt32();
        LOAD_SETTING(generalSettings, assetCacheTimeout, General_AssetCache_Timeout, AsUInt32);

        LOAD_SETTING(generalSettings, autoConversion, General_AutoConvertation, AsBool);
        generalSettings->renderBackend = GetEnumValue(General_RenderBackend, generalSettings->renderBackend);
        LOAD_SETTING(generalSettings, wheelMoveCamera, General_Mouse_WheelMoveCamera, AsBool);
        LOAD_SETTING(generalSettings, wheelMoveIntensity, General_Mouse_WheelMoveIntensity, AsFloat);
        LOAD_SETTING(generalSettings, invertWheel, General_Mouse_InvertWheel, AsBool);

        LOAD_SETTING(globalSceneSettings, gridStep, Scene_GridStep, AsFloat);
        LOAD_SETTING(globalSceneSettings, gridSize, Scene_GridSize, AsFloat);
        LOAD_SETTING(globalSceneSettings, cameraSpeed0, Scene_CameraSpeed0, AsFloat);
        LOAD_SETTING(globalSceneSettings, cameraSpeed1, Scene_CameraSpeed1, AsFloat);
        LOAD_SETTING(globalSceneSettings, cameraSpeed2, Scene_CameraSpeed2, AsFloat);
        LOAD_SETTING(globalSceneSettings, cameraSpeed3, Scene_CameraSpeed3, AsFloat);
        LOAD_SETTING(globalSceneSettings, cameraFOV, Scene_CameraFOV, AsFloat);
        LOAD_SETTING(globalSceneSettings, cameraNear, Scene_CameraNear, AsFloat);
        LOAD_SETTING(globalSceneSettings, cameraFar, Scene_CameraFar, AsFloat);
        LOAD_SETTING(globalSceneSettings, heightOnLandscape, Scene_CameraHeightOnLandscape, AsFloat);
        LOAD_SETTING(globalSceneSettings, heightOnLandscapeStep, Scene_CameraHeightOnLandscapeStep, AsFloat);
        LOAD_SETTING(globalSceneSettings, selectionSequent, Scene_SelectionSequent, AsBool);
        LOAD_SETTING(globalSceneSettings, selectionOnClick, Scene_SelectionOnClick, AsBool);
        globalSceneSettings->selectionDrawMode = GetEnumValue(Scene_SelectionDrawMode, globalSceneSettings->selectionDrawMode);
        LOAD_SETTING(globalSceneSettings, modificationByGizmoOnly, Scene_ModificationByGizmoOnly, AsBool);
        LOAD_SETTING(globalSceneSettings, gizmoScale, Scene_GizmoScale, AsFloat);
        LOAD_SETTING(globalSceneSettings, debugBoxScale, Scene_DebugBoxScale, AsFloat);
        LOAD_SETTING(globalSceneSettings, debugBoxUserScale, Scene_DebugBoxUserScale, AsFloat);
        LOAD_SETTING(globalSceneSettings, debugBoxParticleScale, Scene_DebugBoxParticleScale, AsFloat);
        LOAD_SETTING(globalSceneSettings, debugBoxWaypointScale, Scene_DebugBoxWaypointScale, AsFloat);
        LOAD_SETTING(globalSceneSettings, dragAndDropWithShift, Scene_DragAndDropWithShift, AsBool);
        LOAD_SETTING(globalSceneSettings, autoSelectNewEntity, Scene_AutoselectNewEntities, AsBool);
        LOAD_SETTING(globalSceneSettings, saveEmitters, Scene_SaveEmitters, AsBool);
        LOAD_SETTING(globalSceneSettings, saveStaticOcclusion, Scene_SaveStaticOcclusion, AsBool);
        LOAD_SETTING(globalSceneSettings, defaultCustomColorIndex, Scene_DefaultCustomColorIndex, AsUInt32);
        LOAD_SETTING(globalSceneSettings, drawSoundObjects, Scene_Sound_SoundObjectDraw, AsBool);
        LOAD_SETTING(globalSceneSettings, soundObjectBoxColor, Scene_Sound_SoundObjectBoxColor, AsColor);
        LOAD_SETTING(globalSceneSettings, soundObjectSphereColor, Scene_Sound_SoundObjectSphereColor, AsColor);
        globalSceneSettings->grabSizeWidth = GetValue(Scene_Grab_Size_Width, static_cast<DAVA::int32>(globalSceneSettings->grabSizeWidth)).AsInt32();
        globalSceneSettings->grabSizeHeight = GetValue(Scene_Grab_Size_Width, static_cast<DAVA::int32>(globalSceneSettings->grabSizeHeight)).AsInt32();

        internalSettings->textureViewGPU = GetEnumValue(Internal_TextureViewGPU, internalSettings->textureViewGPU);
        internalSettings->spritesViewGPU = GetEnumValue(Internal_SpriteViewGPU, internalSettings->spritesViewGPU);
        LOAD_SETTING(internalSettings, emitterSaveDir, Internal_ParticleLastSaveEmitterDir, AsFilePath);
        LOAD_SETTING(internalSettings, emitterLoadDir, Internal_ParticleLastSaveEmitterDir, AsFilePath);
        LOAD_SETTING(internalSettings, cubemapLastFaceDir, Internal_CubemapLastFaceDir, AsFilePath);
        LOAD_SETTING(internalSettings, cubemapLastProjDir, Internal_CubemapLastProjDir, AsFilePath);
        internalSettings->materialLightViewMode = GetEnumValue(Internal_MaterialsLightViewMode, internalSettings->materialLightViewMode);
        LOAD_SETTING(internalSettings, materialShowLightmapCanvas, Internal_MaterialsShowLightmapCanvas, AsBool);
        LOAD_SETTING(internalSettings, lodEditorSceneMode, Internal_LODEditor_Mode, AsBool);
        LOAD_SETTING(internalSettings, lodEditorRecursive, Internal_LODEditor_Recursive, AsBool);
        internalSettings->runActionEventType = GetEnumValue(Internal_RunActionEventWidget_CurrentType, internalSettings->runActionEventType);
        LOAD_SETTING(internalSettings, beastLightmapsDefaultDir, Internal_Beast_LightmapsDefaultDir, AsString);
        LOAD_SETTING(internalSettings, imageSplitterPath, Internal_ImageSplitterPath, AsString);
        LOAD_SETTING(internalSettings, imageSplitterPathSpecular, Internal_ImageSplitterPathSpecular, AsString);

        LOAD_SETTING(internalSettings, enableSound, Internal_EnableSounds, AsBool);
        LOAD_SETTING(internalSettings, gizmoEnabled, Internal_GizmoEnabled, AsBool);
        LOAD_SETTING(internalSettings, validateMatrices, Internal_Validate_Matrices, AsBool);
        LOAD_SETTING(internalSettings, validateSameNames, Internal_Validate_SameNames, AsBool);
        LOAD_SETTING(internalSettings, validateCollisionProperties, Internal_Validate_CollisionProperties, AsBool);
        LOAD_SETTING(internalSettings, validateTextureRelevance, Internal_Validate_TexturesRelevance, AsBool);
        LOAD_SETTING(internalSettings, validateMaterialGroups, Internal_Validate_MaterialGroups, AsBool);
        LOAD_SETTING(internalSettings, validateShowConsole, Internal_Validate_ShowConsole, AsBool);

#undef LOAD_SETTING

        {
            auto iter = settingsMap2.find(DAVA::FastName("ThemeName"));
            if (iter != settingsMap2.end())
            {
                themeSettings->SetTheme(static_cast<ThemesSettings::eTheme>(iter->second.AsInt64()), DAVA::PlatformApi::Qt::GetApplication());
            }
        }

        auto iter = settingsMap.find(FastName(Internal_LogWidget));
        if (iter != settingsMap.end())
        {
            VariantType value = iter->second;
            int32 size = value.AsByteArraySize();
            const uint8* byteArray = value.AsByteArray();
            internalSettings->logWidgetState = QByteArray(reinterpret_cast<const char*>(byteArray), size);
        }

        // convert module specific settings
        {
            PropertiesItem item = rootSettingsNode.CreateSubHolder("VersionsInfo");
            item.Set("EditorVersion", GetValue(Internal_EditorVersion, String("")).AsString());
        }

        {
            PropertiesItem item = rootSettingsNode.CreateSubHolder("ColorPickerDialogProperties");
            colorPickerSettings->maxMultiplier = item.Get("CPD_maxMultiplier", colorPickerSettings->maxMultiplier);
            colorPickerSettings->customPalette = item.Get("CPD_palette", colorPickerSettings->customPalette);
            colorPickerSettings->dialogGeometry = item.Get("CPD_geometry", colorPickerSettings->dialogGeometry);
        }
    }

private:
    template <typename T>
    DAVA::VariantType GetValue(const DAVA::String& key, T defaulValue) const
    {
        auto iter = settingsMap.find(DAVA::FastName(key));
        if (iter == settingsMap.end())
        {
            return DAVA::VariantType(defaulValue);
        }

        return iter->second;
    }

    template <typename T>
    T GetEnumValue(const DAVA::String& key, T defaultValue) const
    {
        DAVA::VariantType v = GetValue(key, static_cast<DAVA::int32>(defaultValue));
        return static_cast<T>(DAVA::VariantType::Convert(v, DAVA::VariantType::TYPE_INT32).AsInt32());
    }

    DAVA::VariantType CustomTextureViewGPULoad(const DAVA::String& paramName, const DAVA::VariantType& srcValue)
    {
        if (DAVA::VariantType::TYPE_INT32 == srcValue.GetType() && paramName == Internal_TextureViewGPU)
        {
            DAVA::eGPUFamily gpuFamilyRead = DAVA::GPUFamilyDescriptor::ConvertValueToGPU(srcValue.AsInt32());
            DAVA::uint32 valueToVariant = static_cast<DAVA::uint32>(gpuFamilyRead);
            DAVA::VariantType dstValue(valueToVariant);
            return dstValue;
        }
        return srcValue;
    }

    DAVA::UnorderedMap<DAVA::FastName, DAVA::VariantType> settingsMap;
    DAVA::UnorderedMap<DAVA::FastName, DAVA::VariantType> settingsMap2;

    const DAVA::String settingsFilePath = "~doc:/ResourceEditorOptions.archive";
    const DAVA::String settingsFilePath2 = "~doc:/ResourceEditorSettings.archive";

    const DAVA::String General_RecentFilesCount = "General/RecentFilesCount";
    const DAVA::String General_RecentProjectsCount = "General/RecentProjectsCount";
    const DAVA::String General_ReloadParticlesOnPojectOpening = "General/ReloadParticlesOnProjectOpening";
    const DAVA::String General_PreviewEnabled = "General/PreviewEnabled";
    const DAVA::String General_OpenByDBClick = "General/OpenByDoubleClick";
    const DAVA::String General_CompressionQuality = "General/CompressionQuality";
    const DAVA::String General_ShowErrorDialog = "General/ShowDialogOnError";
    const DAVA::String General_MaterialEditor_SwitchColor0 = "General/MaterialEditor/SwitchColor0";
    const DAVA::String General_MaterialEditor_SwitchColor1 = "General/MaterialEditor/SwitchColor1";
    const DAVA::String General_MaterialEditor_LodColor0 = "General/MaterialEditor/LodColor0";
    const DAVA::String General_MaterialEditor_LodColor1 = "General/MaterialEditor/LodColor1";
    const DAVA::String General_MaterialEditor_LodColor2 = "General/MaterialEditor/LodColor2";
    const DAVA::String General_MaterialEditor_LodColor3 = "General/MaterialEditor/LodColor3";
    const DAVA::String General_LODEditor_LodColor0 = "General/LODEditor/LodColor0";
    const DAVA::String General_LODEditor_LodColor1 = "General/LODEditor/LodColor1";
    const DAVA::String General_LODEditor_LodColor2 = "General/LODEditor/LodColor2";
    const DAVA::String General_LODEditor_LodColor3 = "General/LODEditor/LodColor3";
    const DAVA::String General_LODEditor_InactiveColor = "General/LODEditor/InactiveColor";
    const DAVA::String General_LODEditor_FitSliders = "General/LODEditor/FitSlidersToMaximumDistance";
    const DAVA::String General_ParticleEditor_ParticleDebugAlphaTheshold = "General/ParticleEditor/ParticleDebugAlphaTheshold";
    const DAVA::String General_HeighMaskTool_Color0 = "General/HeighMaskTool/Color0";
    const DAVA::String General_HeighMaskTool_Color1 = "General/HeighMaskTool/Color1";
    const DAVA::String General_AssetCache_UseCache = "General/AssetCache/UseCache";
    const DAVA::String General_AssetCache_Ip = "General/AssetCache/IP";
    const DAVA::String General_AssetCache_Port = "General/AssetCache/Port";
    const DAVA::String General_AssetCache_Timeout = "General/AssetCache/Timeout";
    const DAVA::String General_AutoConvertation = "General/TextureBrowser/AutoConvertation";
    const DAVA::String General_RenderBackend = "General/Render/Backend";
    const DAVA::String General_Mouse_InvertWheel = "General/Mouse/InvertWheel";
    const DAVA::String General_Mouse_WheelMoveCamera = "General/Mouse/WheelMoveCamera";
    const DAVA::String General_Mouse_WheelMoveIntensity = "General/Mouse/WheelMoveIntensity";
    const DAVA::String Scene_GridStep = "Scene/GridStep";
    const DAVA::String Scene_GridSize = "Scene/GridSize";
    const DAVA::String Scene_CameraSpeed0 = "Scene/CameraSpeed0";
    const DAVA::String Scene_CameraSpeed1 = "Scene/CameraSpeed1";
    const DAVA::String Scene_CameraSpeed2 = "Scene/CameraSpeed2";
    const DAVA::String Scene_CameraSpeed3 = "Scene/CameraSpeed3";
    const DAVA::String Scene_CameraFOV = "Scene/CameraFOV";
    const DAVA::String Scene_CameraNear = "Scene/CameraNear";
    const DAVA::String Scene_CameraFar = "Scene/CameraFar";
    const DAVA::String Scene_CameraHeightOnLandscape = "Scene/HeightOnLandscape";
    const DAVA::String Scene_CameraHeightOnLandscapeStep = "Scene/HeightOnLandscapeStep";
    const DAVA::String Scene_SelectionSequent = "Scene/SelectionSequent";
    const DAVA::String Scene_SelectionOnClick = "Scene/SelectionOnClick";
    const DAVA::String Scene_SelectionDrawMode = "Scene/SelectionDrawMode";
    const DAVA::String Scene_CollisionDrawMode = "Scene/CollisionDrawMode";
    const DAVA::String Scene_ModificationByGizmoOnly = "Scene/ModificationByGizmoOnly";
    const DAVA::String Scene_GizmoScale = "Scene/GizmoScale";
    const DAVA::String Scene_DebugBoxScale = "Scene/DebugBoxScale";
    const DAVA::String Scene_DebugBoxUserScale = "Scene/DebugBoxUserScale";
    const DAVA::String Scene_DebugBoxParticleScale = "Scene/DebugBoxParticleScale";
    const DAVA::String Scene_DebugBoxWaypointScale = "Scene/DebugBoxWaypointScale";
    const DAVA::String Scene_DragAndDropWithShift = "Scene/Drag&DropInTreeWithShift";
    const DAVA::String Scene_AutoselectNewEntities = "Scene/AutoselectNewEnities";
    const DAVA::String Scene_SaveEmitters = "Scene/SaveEmittersWithScene";
    const DAVA::String Scene_SaveStaticOcclusion = "Scene/SaveAfterStaticOcclusion";
    const DAVA::String Scene_DefaultCustomColorIndex = "Scene/DefaultCustomColorIndex";
    const DAVA::String Scene_Sound_SoundObjectDraw = "Scene/Sound/SoundObjectDraw";
    const DAVA::String Scene_Sound_SoundObjectBoxColor = "Scene/Sound/SoundObjectBoxColor";
    const DAVA::String Scene_Sound_SoundObjectSphereColor = "Scene/Sound/SoundObjectSphereColor";
    const DAVA::String Scene_Grab_Size_Width = "Scene/Grab Scene/Width";
    const DAVA::String Scene_Grab_Size_Height = "Scene/Grab Scene/Height";
    const DAVA::String Scene_Slot_Box_Color = "Scene/Slot's debug draw/Box color";
    const DAVA::String Scene_Slot_Box_Edges_Color = "Scene/Slot's debug draw/Box edges color";
    const DAVA::String Scene_Slot_Pivot_Color = "Scene/Slot's debug draw/Pivot color";
    const DAVA::String Internal_TextureViewGPU = "Internal/TextureViewGPU";
    const DAVA::String Internal_SpriteViewGPU = "Internal/SpriteViewGPU";
    const DAVA::String Internal_EditorVersion = "Internal/EditorVersion";
    const DAVA::String Internal_CubemapLastFaceDir = "Internal/CubemapLastFaceDir";
    const DAVA::String Internal_CubemapLastProjDir = "Internal/CubemapLastProjDir";
    const DAVA::String Internal_ParticleLastSaveEmitterDir = "Internal/ParticleLastEmitterDir";
    const DAVA::String Internal_ParticleLastLoadEmitterDir = "Internal/ParticleLastLoadEmitterDir";
    const DAVA::String Internal_MaterialsLightViewMode = "Internal/MaterialsLightViewMode";
    const DAVA::String Internal_MaterialsShowLightmapCanvas = "Internal/MaterialsShowLightmapCanvas";
    const DAVA::String Internal_LODEditor_Mode = "Internal/LODEditorMode";
    const DAVA::String Internal_LODEditor_Recursive = "Internal/LodEditor/Recursive";
    const DAVA::String Internal_RunActionEventWidget_CurrentType = "Internal/RunActionEventWidget/CurrentType";
    const DAVA::String Internal_Beast_LightmapsDefaultDir = "Internal/Beast/LightmapsDefaultDir";
    const DAVA::String Internal_ImageSplitterPath = "Internal/ImageSplitterPath";
    const DAVA::String Internal_ImageSplitterPathSpecular = "Internal/ImageSplitterPath_specular";
    const DAVA::String Internal_EnableSounds = "Internal/EnableSounds";
    const DAVA::String Internal_GizmoEnabled = "Internal/GizmoEnabled";
    const DAVA::String Internal_Validate_Matrices = "Internal/ValidateMatrices";
    const DAVA::String Internal_Validate_SameNames = "Internal/ValidateSameNames";
    const DAVA::String Internal_Validate_CollisionProperties = "Internal/ValidateCollisionProperties";
    const DAVA::String Internal_Validate_TexturesRelevance = "Internal/ValidateTexturesRelevance";
    const DAVA::String Internal_Validate_MaterialGroups = "Internal/ValidateMaterialGroups";
    const DAVA::String Internal_Validate_ShowConsole = "Internal/ValidateShowConsole";
    const DAVA::String Internal_LogWidget = "Internal/LogWidget";

    const DAVA::String General_ColorMultiplyMax = "General/ColorPicker/Maximum multiplier";
    const DAVA::String Internal_CustomPalette = "Internal/CustomPalette";
};

void ConvertToInitVersion(const DAVA::TArc::PropertiesHolder& rootNode, DAVA::TArc::ContextAccessor* accessor)
{
    OldSettingsConverter converter;
    converter.Do(rootNode, accessor);
}
} // namespace SettingsConverterDetail

void ConvertSettingsIfNeeded(const DAVA::TArc::PropertiesHolder& rootNode, DAVA::TArc::ContextAccessor* accessor)
{
    using namespace SettingsConverterDetail;
    DAVA::uint32 settingsVersion = 0;
    {
        DAVA::TArc::PropertiesItem verionsInfo = rootNode.CreateSubHolder("VersionsInfo");
        settingsVersion = verionsInfo.Get<DAVA::uint32>("SettingsVersion", 0);
    }
    if (settingsVersion == 0)
    {
        ConvertToInitVersion(rootNode, accessor);
    }

    rootNode.CreateSubHolder("VersionsInfo").Set("SettingsVersion", CURRENT_VERSION);
}
