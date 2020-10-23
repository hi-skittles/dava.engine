#pragma once

#include "Classes/Qt/Scene/System/EditorMaterialSystem.h"

#include <TArc/DataProcessing/SettingsNode.h>
#include <TArc/Qt/QtByteArray.h>

#include <AssetCache/AssetCacheConstants.h>
#include <TextureCompression/TextureConverter.h>

#include <Scene3D/Components/ActionComponent.h>
#include <Reflection/Reflection.h>
#include <Math/Color.h>
#include <Base/BaseTypes.h>

namespace DAVA
{
namespace TArc
{
class PropertiesItem;
} // namespace TArc
} // namespace DAVA

enum class RenderingBackend
{
    DX11 = 0,
    DX9,
    OpenGL
};

class GeneralSettings : public DAVA::TArc::SettingsNode
{
public:
    bool reloadParticlesOnProjectOpening = true;
    bool previewEnabled = false;
    DAVA::TextureConverter::eConvertQuality compressionQuality = DAVA::TextureConverter::ECQ_DEFAULT;
    bool showErrorDialog = true;
    DAVA::uint32 recentScenesCount = 15;

    // Material Editor settings
    DAVA::Color materialEditorSwitchColor0 = DAVA::Color(0.0f, 1.0f, 0.0f, 1.0f);
    DAVA::Color materialEditorSwitchColor1 = DAVA::Color(1.0f, 0.0f, 0.0f, 1.0f);
    DAVA::Color materialEditorLodColor0 = DAVA::Color(0.9f, 0.9f, 0.9f, 1.0f);
    DAVA::Color materialEditorLodColor1 = DAVA::Color(0.7f, 0.7f, 0.7f, 1.0f);
    DAVA::Color materialEditorLodColor2 = DAVA::Color(0.5f, 0.5f, 0.5f, 1.0f);
    DAVA::Color materialEditorLodColor3 = DAVA::Color(0.3f, 0.3f, 0.3f, 1.0f);

    // Particle Editor settings
    DAVA::float32 particleDebugAlphaTheshold = 0.05f;

    // Lod Editor settings
    DAVA::Color lodEditorLodColor0 = DAVA::Color(0.2f, 0.35f, 0.62f, 1.0f);
    DAVA::Color lodEditorLodColor1 = DAVA::Color(0.25f, 0.45f, 0.78f, 1.0f);
    DAVA::Color lodEditorLodColor2 = DAVA::Color(0.33f, 0.56f, 0.97f, 1.0f);
    DAVA::Color lodEditorLodColor3 = DAVA::Color(0.62f, 0.75f, 0.98f, 1.0f);
    DAVA::Color inactiveColor = DAVA::Color(0.59f, 0.59f, 0.59f, 1.0f);
    bool fitSliders = false;

    // Height mask tools settings
    DAVA::Color heightMaskColor0 = DAVA::Color(0.5f, 0.5f, 0.5f, 1.0f);
    DAVA::Color heightMaskColor1 = DAVA::Color(0.0f, 0.0f, 0.0f, 1.0f);

    // Asset cache settings
    bool useAssetCache = false;
    DAVA::String assetCacheIP = "";
    DAVA::uint16 assetCachePort = DAVA::AssetCache::ASSET_SERVER_PORT;
    DAVA::uint32 assetCacheTimeout = 10;

    // Texture browser settings
    bool autoConversion = true;

    // Renderer settings
    RenderingBackend renderBackend = RenderingBackend::OpenGL;

    // Mouse settings
    bool wheelMoveCamera = true;
    DAVA::float32 wheelMoveIntensity = 180.0f;
    bool invertWheel = false;

    void Load(const DAVA::TArc::PropertiesItem& settingsNode) override;

    DAVA_VIRTUAL_REFLECTION(GeneralSettings, DAVA::TArc::SettingsNode);
};

class CommonInternalSettings : public DAVA::TArc::SettingsNode
{
public:
    DAVA::eGPUFamily textureViewGPU = DAVA::GPU_ORIGIN;
    DAVA::eGPUFamily spritesViewGPU = DAVA::GPU_ORIGIN;
    DAVA::FilePath cubemapLastFaceDir;
    DAVA::FilePath cubemapLastProjDir;
    DAVA::FilePath emitterSaveDir;
    DAVA::FilePath emitterLoadDir;
    EditorMaterialSystem::MaterialLightViewMode materialLightViewMode = EditorMaterialSystem::LIGHTVIEW_ALL;
    bool materialShowLightmapCanvas = false;
    bool lodEditorSceneMode = false;
    bool lodEditorRecursive = false;
    DAVA::ActionComponent::Action::eEvent runActionEventType = DAVA::ActionComponent::Action::EVENT_SWITCH_CHANGED;
    DAVA::String beastLightmapsDefaultDir = DAVA::String("lightmaps");
    DAVA::String imageSplitterPath = DAVA::String("");
    DAVA::String imageSplitterPathSpecular = DAVA::String("");
    bool enableSound = true;
    bool gizmoEnabled = true;
    bool validateMatrices = true;
    bool validateSameNames = true;
    bool validateCollisionProperties = true;
    bool validateTextureRelevance = true;
    bool validateMaterialGroups = true;
    bool validateShowConsole = true;
    QByteArray logWidgetState;

    DAVA_VIRTUAL_REFLECTION(CommonInternalSettings, DAVA::TArc::SettingsNode);
};
