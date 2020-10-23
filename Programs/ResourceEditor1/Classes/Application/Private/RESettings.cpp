#include "Classes/Application/RESettings.h"

#include <Reflection/ReflectionRegistrator.h>
#include <Base/GlobalEnum.h>

ENUM_DECLARE(RenderingBackend)
{
#if defined(__DAVAENGINE_WIN32__)
    // Uncomment this line to allow DX11 backend
    //ENUM_ADD_DESCR(static_cast<int>(RenderingBackend::DX11), "DirectX 11");
    ENUM_ADD_DESCR(static_cast<int>(RenderingBackend::DX9), "DirectX 9");
#endif
    ENUM_ADD_DESCR(static_cast<int>(RenderingBackend::OpenGL), "OpenGL");
}

DAVA_VIRTUAL_REFLECTION_IMPL(GeneralSettings)
{
    DAVA::ReflectionRegistrator<GeneralSettings>::Begin()[DAVA::M::DisplayName("General"), DAVA::M::SettingsSortKey(100)]
    .ConstructorByPointer()
    .Field("ReloadParticlesOnProjectOpening", &GeneralSettings::reloadParticlesOnProjectOpening)[DAVA::M::DisplayName("Convert particles on project opening")]
    .Field("PreviewEnabled", &GeneralSettings::previewEnabled)[DAVA::M::DisplayName("Show scene preview")]
    .Field("CompressionQuality", &GeneralSettings::compressionQuality)[DAVA::M::DisplayName("Compression quality"), DAVA::M::EnumT<DAVA::TextureConverter::eConvertQuality>()]
    .Field("ShowErrorDialog", &GeneralSettings::showErrorDialog)[DAVA::M::DisplayName("Show error dialog")]
    .Field("recentScenesCount", &GeneralSettings::recentScenesCount)[DAVA::M::DisplayName("Number of recent scenes"), DAVA::M::Range(0, 50, 1)]
    .Field("materialEditorSwitchColor0", &GeneralSettings::materialEditorSwitchColor0)[DAVA::M::DisplayName("Switch 0 color"), DAVA::M::Group("Material Editor")]
    .Field("materialEditorSwitchColor1", &GeneralSettings::materialEditorSwitchColor1)[DAVA::M::DisplayName("Switch 1 color"), DAVA::M::Group("Material Editor")]
    .Field("materialEditorLod0Color", &GeneralSettings::materialEditorLodColor0)[DAVA::M::DisplayName("Lod 0 color"), DAVA::M::Group("Material Editor")]
    .Field("materialEditorLod1Color", &GeneralSettings::materialEditorLodColor1)[DAVA::M::DisplayName("Lod 1 color"), DAVA::M::Group("Material Editor")]
    .Field("materialEditorLod2Color", &GeneralSettings::materialEditorLodColor2)[DAVA::M::DisplayName("Lod 2 color"), DAVA::M::Group("Material Editor")]
    .Field("materialEditorLod3Color", &GeneralSettings::materialEditorLodColor3)[DAVA::M::DisplayName("Lod 3 color"), DAVA::M::Group("Material Editor")]
    .Field("ParticleDebugAlphaThreshold", &GeneralSettings::particleDebugAlphaTheshold)[DAVA::M::DisplayName("Particle alpha threshold"), DAVA::M::Group("Particle editor")]
    .Field("lodEditorLodColor0", &GeneralSettings::lodEditorLodColor0)[DAVA::M::DisplayName("Lod 0 color"), DAVA::M::Group("LOD Editor")]
    .Field("lodEditorLodColor1", &GeneralSettings::lodEditorLodColor1)[DAVA::M::DisplayName("Lod 1 color"), DAVA::M::Group("LOD Editor")]
    .Field("lodEditorLodColor2", &GeneralSettings::lodEditorLodColor2)[DAVA::M::DisplayName("Lod 2 color"), DAVA::M::Group("LOD Editor")]
    .Field("lodEditorLodColor3", &GeneralSettings::lodEditorLodColor3)[DAVA::M::DisplayName("Lod 3 color"), DAVA::M::Group("LOD Editor")]
    .Field("inactiveColor", &GeneralSettings::inactiveColor)[DAVA::M::DisplayName("Inactive color"), DAVA::M::Group("LOD Editor")]
    .Field("fitSliders", &GeneralSettings::fitSliders)[DAVA::M::DisplayName("Fit sliders to maximum distance"), DAVA::M::Group("LOD Editor")]
    .Field("heightMaskColor0", &GeneralSettings::heightMaskColor0)[DAVA::M::DisplayName("Color 0"), DAVA::M::Group("Height Mask Tool")]
    .Field("heightMaskColor1", &GeneralSettings::heightMaskColor1)[DAVA::M::DisplayName("Color 1"), DAVA::M::Group("Height Mask Tool")]
    .Field("useAssetCache", &GeneralSettings::useAssetCache)[DAVA::M::DisplayName("Use cache"), DAVA::M::Group("Asset Cache")]
    .Field("assetCacheIP", &GeneralSettings::assetCacheIP)[DAVA::M::DisplayName("IP"), DAVA::M::Group("Asset Cache")]
    .Field("assetCachePort", &GeneralSettings::assetCachePort)[DAVA::M::DisplayName("Port"), DAVA::M::Group("Asset Cache")]
    .Field("assetCacheTimeout", &GeneralSettings::assetCacheTimeout)[DAVA::M::DisplayName("Timeout"), DAVA::M::Group("Asset Cache")]
    .Field("autoConversion", &GeneralSettings::autoConversion)[DAVA::M::DisplayName("Convert automatically"), DAVA::M::Group("Texture Browser")]
    .Field("renderBackend", &GeneralSettings::renderBackend)[
#if defined(DEPLOY_BUILD)
    DAVA::M::HiddenField(),
#endif
    DAVA::M::DisplayName("Backend"), DAVA::M::Group("Renderer"), DAVA::M::EnumT<RenderingBackend>()]
    .Field("wheelMoveCamera", &GeneralSettings::wheelMoveCamera)[DAVA::M::DisplayName("Move camera on Wheel"), DAVA::M::Group("Mouse")]
    .Field("wheelMoveIntensity", &GeneralSettings::wheelMoveIntensity)[DAVA::M::DisplayName("Move intensity on Wheel"), DAVA::M::Group("Mouse")]
    .Field("invertWheel", &GeneralSettings::invertWheel)[DAVA::M::DisplayName("Invert Wheel"), DAVA::M::Group("Mouse")]
    .End();
}

void GeneralSettings::Load(const DAVA::TArc::PropertiesItem& settingsNode)
{
    SettingsNode::Load(settingsNode);
#if defined(DEPLOY_BUILD)
    renderBackend = RenderingBackend::OpenGL;
#endif
}

DAVA_VIRTUAL_REFLECTION_IMPL(CommonInternalSettings)
{
    DAVA::ReflectionRegistrator<CommonInternalSettings>::Begin()[DAVA::M::HiddenField()]
    .ConstructorByPointer()
    .Field("textureViewGPU", &CommonInternalSettings::textureViewGPU)
    .Field("spritesViewGPU", &CommonInternalSettings::spritesViewGPU)
    .Field("cubemapLastFaceDir", &CommonInternalSettings::cubemapLastFaceDir)
    .Field("cubemapLastProjDir", &CommonInternalSettings::cubemapLastProjDir)
    .Field("emitterSaveDir", &CommonInternalSettings::emitterSaveDir)
    .Field("emitterLoadDir", &CommonInternalSettings::emitterLoadDir)
    .Field("materialLightViewMode", &CommonInternalSettings::materialLightViewMode)
    .Field("materialShowLightmapCanvas", &CommonInternalSettings::materialShowLightmapCanvas)
    .Field("lodEditorSceneMode", &CommonInternalSettings::lodEditorSceneMode)
    .Field("lodEditorRecursive", &CommonInternalSettings::lodEditorRecursive)
    .Field("runActionEventType", &CommonInternalSettings::runActionEventType)
    .Field("beastLightmapsDefaultDir", &CommonInternalSettings::beastLightmapsDefaultDir)
    .Field("imageSplitterPath", &CommonInternalSettings::imageSplitterPath)
    .Field("imageSplitterPathSpecular", &CommonInternalSettings::imageSplitterPathSpecular)
    .Field("enableSound", &CommonInternalSettings::enableSound)
    .Field("gizmoEnabled", &CommonInternalSettings::gizmoEnabled)
    .Field("validateMatrices", &CommonInternalSettings::validateMatrices)
    .Field("validateSameNames", &CommonInternalSettings::validateSameNames)
    .Field("validateCollisionProperties", &CommonInternalSettings::validateCollisionProperties)
    .Field("validateTextureRelevance", &CommonInternalSettings::validateTextureRelevance)
    .Field("validateMaterialGroups", &CommonInternalSettings::validateMaterialGroups)
    .Field("validateShowConsole", &CommonInternalSettings::validateShowConsole)
    .Field("logWidgetState", &CommonInternalSettings::logWidgetState)
    .End();
}
