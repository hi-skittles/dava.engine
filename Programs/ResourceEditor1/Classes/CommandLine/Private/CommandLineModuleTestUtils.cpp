#include "CommandLine/Private/CommandLineModuleTestUtils.h"
#include "Classes/Qt/Scene/System/BeastSystem.h"

#include "Utils/TextureDescriptor/RETextureDescriptorUtils.h"

#include <Engine/Engine.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/YamlNode.h>
#include <FileSystem/YamlParser.h>
#include <FileSystem/YamlEmitter.h>
#include <Render/3D/PolygonGroup.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Highlevel/Mesh.h>
#include <Render/Highlevel/Vegetation/VegetationRenderObject.h>
#include <Render/Image/Image.h>
#include <Render/Image/ImageSystem.h>
#include <Render/Material/NMaterial.h>
#include <Scene3D/Lod/LodComponent.h>
#include <Scene3D/Components/CustomPropertiesComponent.h>
#include <Scene3D/Components/CameraComponent.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/LightComponent.h>
#include <Scene3D/Components/StaticOcclusionComponent.h>
#include <Scene3D/Components/SwitchComponent.h>
#include <Scene3D/Components/RenderComponent.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Scene3D/Components/TextComponent.h>
#include <Scene3D/Components/Waypoint/WaypointComponent.h>
#include <Scene3D/Components/Waypoint/EdgeComponent.h>
#include <Scene3D/Scene.h>
#include <Utils/Random.h>
#include <Utils/StringFormat.h>

namespace CommandLineModuleTestUtils
{
const DAVA::String SceneBuilder::tagChina = ".china";
const DAVA::String SceneBuilder::tagJapan = ".japan";
const DAVA::String SceneBuilder::tagDefault = "";
const DAVA::String SceneBuilder::chinaSlotDir = "China.slot/";
const DAVA::String SceneBuilder::defaultSlotDir = "Default.slot/";

namespace Detail
{
using namespace DAVA;

bool CreateImageFile(const FilePath& imagePathname, uint32 width, uint32 height, PixelFormat format, uint8 color)
{
    ScopedPtr<Image> image(Image::Create(width, height, format));

    Memset(image->data, color, image->dataSize);

    eErrorCode saved = ImageSystem::Save(imagePathname, image, image->format);
    return (saved == eErrorCode::SUCCESS);
}

bool CreateHeightmapFile(const FilePath& heightmapPathname, uint32 width, PixelFormat format)
{
    DVASSERT(format == PixelFormat::FORMAT_A8 || format == PixelFormat::FORMAT_A16);
    return CreateImageFile(heightmapPathname, width, width, format, GetEngineContext()->random->Rand(255));
}

NMaterial* CreateMaterial(const FastName& materialName, const FastName& fxName)
{
    NMaterial* material = new NMaterial();
    material->SetMaterialName(materialName);
    material->SetFXName(fxName);
    return material;
}

PolygonGroup* CreatePolygonGroup()
{
    PolygonGroup* renderData = new PolygonGroup();
    renderData->AllocateData(EVF_VERTEX | EVF_TEXCOORD0, 4, 6);
    renderData->SetPrimitiveType(rhi::PrimitiveType::PRIMITIVE_TRIANGLELIST);
    renderData->SetCoord(0, Vector3(-1.0f, -1.0f, 0.0f));
    renderData->SetCoord(1, Vector3(1.0f, -1.0f, 0.0f));
    renderData->SetCoord(2, Vector3(-1.0f, 1.0f, 0.0f));
    renderData->SetCoord(3, Vector3(1.0f, 1.0f, 0.0f));
    renderData->SetTexcoord(0, 0, Vector2(0.0f, 0.0f));
    renderData->SetTexcoord(0, 1, Vector2(1.0f, 0.0f));
    renderData->SetTexcoord(0, 2, Vector2(0.0f, 1.0f));
    renderData->SetTexcoord(0, 3, Vector2(1.0f, 1.0f));
    renderData->SetIndex(0, 0);
    renderData->SetIndex(1, 1);
    renderData->SetIndex(2, 2);
    renderData->SetIndex(3, 2);
    renderData->SetIndex(4, 1);
    renderData->SetIndex(5, 3);
    renderData->BuildBuffers();

    return renderData;
}

bool CreateTextureFiles(const FilePath& texturePathname, uint32 width, uint32 height, PixelFormat format, uint8 color, const String& tag = "")
{
    FilePath taggedTexturePath = texturePathname;
    taggedTexturePath.ReplaceBasename(texturePathname.GetBasename() + tag);

    FilePath pngPathname = FilePath::CreateWithNewExtension(taggedTexturePath, ".png");
    if (CreateImageFile(pngPathname, width, height, format, color))
    {
        RETextureDescriptorUtils::CreateOrUpdateDescriptor(pngPathname);

        std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(taggedTexturePath));
        if (descriptor)
        {
            descriptor->compression[eGPUFamily::GPU_POWERVR_IOS].format = PixelFormat::FORMAT_RGBA8888;
            descriptor->compression[eGPUFamily::GPU_POWERVR_ANDROID].format = PixelFormat::FORMAT_RGBA5551;
            descriptor->compression[eGPUFamily::GPU_ADRENO].format = PixelFormat::FORMAT_RGBA4444;
            descriptor->compression[eGPUFamily::GPU_MALI].format = PixelFormat::FORMAT_RGB888;
            descriptor->compression[eGPUFamily::GPU_TEGRA].format = PixelFormat::FORMAT_RGB565;
            descriptor->compression[eGPUFamily::GPU_DX11].format = PixelFormat::FORMAT_A8;

            descriptor->compression[eGPUFamily::GPU_POWERVR_IOS].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
            descriptor->compression[eGPUFamily::GPU_POWERVR_ANDROID].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
            descriptor->compression[eGPUFamily::GPU_ADRENO].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
            descriptor->compression[eGPUFamily::GPU_MALI].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
            descriptor->compression[eGPUFamily::GPU_TEGRA].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
            descriptor->compression[eGPUFamily::GPU_DX11].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
            descriptor->SetGenerateMipmaps(true);
            descriptor->Save();

            return true;
        }
    }
    return false;
}

void CreateR2OCustomProperty(Entity* entity, FilePath pathname)
{
    String entityName = entity->GetName().c_str();

    FilePath folderPathname = pathname.GetDirectory();
    pathname.ReplaceBasename(entityName);

    ScopedPtr<Scene> referenceScene(new Scene);
    ScopedPtr<Entity> referenceEntity(entity->Clone());
    referenceScene->AddNode(referenceEntity);
    referenceScene->SaveScene(pathname, false);

    CustomPropertiesComponent* cp = new CustomPropertiesComponent();
    cp->GetArchive()->SetString("editor.referenceToOwner", pathname.GetAbsolutePathname());
    entity->AddComponent(cp);
}

Entity* CreateLandscapeEnity(const FilePath& scenePathname)
{
    ScopedPtr<Landscape> landscape(new Landscape());
    landscape->SetRenderMode(Landscape::RENDERMODE_NO_INSTANCING);

    // create heightmap
    FilePath heightmapPathname = scenePathname;
    heightmapPathname.ReplaceFilename("landscape.heightmap.png");
    if (CreateHeightmapFile(heightmapPathname, 512u, PixelFormat::FORMAT_A8) == false)
    {
        return nullptr;
    }

    //create geometry
    AABBox3 bboxForLandscape(Vector3(-300.0f, -300.0f, 0.0f), Vector3(300.0f, 300.0f, 50.0f));
    landscape->BuildLandscapeFromHeightmapImage(heightmapPathname, bboxForLandscape);

    //setup textures
    NMaterial* material = landscape->GetMaterial();
    DVASSERT(material != nullptr);
    material->SetQualityGroup(Landscape::LANDSCAPE_QUALITY_NAME);
    material->AddFlag(NMaterialFlagName::FLAG_ILLUMINATION_USED, true);
    material->AddFlag(NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER, true);
    material->AddFlag(NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER, true);

    float32 lighmapSize = 1024.0f;
    material->AddProperty(NMaterialParamName::PARAM_LIGHTMAP_SIZE, &lighmapSize, rhi::ShaderProp::TYPE_FLOAT1, 1);

    auto setupTexture = [&](const String& fileName, const FastName& slotName)
    {
        FilePath textuePathname = scenePathname;
        textuePathname.ReplaceFilename(fileName);
        CreateTextureFiles(textuePathname, 2048u, 2048u, PixelFormat::FORMAT_RGBA8888, GetEngineContext()->random->Rand(255));

        ScopedPtr<Texture> texture(Texture::CreateFromFile(textuePathname));
        material->AddTexture(slotName, texture);
    };

    setupTexture("landscape.colorTexture.tex", Landscape::TEXTURE_COLOR);
    setupTexture("landscape.tileMask.tex", Landscape::TEXTURE_TILEMASK);
    setupTexture("landscape.tileTexture0.tex", Landscape::TEXTURE_TILE);
    setupTexture("landscape.fullTiledTexture.tex", FastName("fullTiledTexture"));

    Entity* entity = new Entity();
    entity->SetName("landscape");
    RenderComponent* rc = new RenderComponent(landscape);
    entity->AddComponent(rc);

    return entity;
}

Entity* CreateWaterEntity(const FilePath& scenePathname)
{
    Entity* entity = new Entity();
    entity->SetName(FastName("water"));

    ScopedPtr<NMaterial> material(CreateMaterial(FastName("water"), NMaterialName::WATER_ALL_QUALITIES));
    material->SetQualityGroup(FastName("Water"));
    ScopedPtr<PolygonGroup> geometry(CreatePolygonGroup());

    ScopedPtr<Mesh> ro(new Mesh());
    ro->AddPolygonGroup(geometry, material);
    RenderComponent* rc = new RenderComponent(ro);
    entity->AddComponent(rc);

    return entity;
}

Entity* CreateSkyEntity(const FilePath& scenePathname)
{
    Entity* entity = new Entity();
    entity->SetName(FastName("sky"));

    ScopedPtr<NMaterial> material(CreateMaterial(FastName("sky"), NMaterialName::SKYOBJECT));
    ScopedPtr<PolygonGroup> geometry(CreatePolygonGroup());

    ScopedPtr<Mesh> ro(new Mesh());
    ro->AddPolygonGroup(geometry, material);
    RenderComponent* rc = new RenderComponent(ro);
    entity->AddComponent(rc);

    return entity;
}

Entity* CreateVegetationEntity(const FilePath& scenePathname)
{
    FilePath customGeometryPathname = scenePathname;
    customGeometryPathname.ReplaceBasename("customGeometry");

    { //create custom geometry
        ScopedPtr<Scene> scene(new Scene());

        ScopedPtr<Entity> vegetationGeometry(new Entity());
        vegetationGeometry->SetName(FastName("variation_0"));

        Vector<FastName> VEGETATION_ENTITY_LAYER_NAMES =
        {
          FastName("layer_0"),
          FastName("layer_1"),
          FastName("layer_2"),
          FastName("layer_3")
        };

        FilePath texturePathname = scenePathname;
        texturePathname.ReplaceFilename("vegetation.texture.tex");
        CreateTextureFiles(texturePathname, 128, 128u, PixelFormat::FORMAT_RGBA8888, GetEngineContext()->random->Rand(255));
        ScopedPtr<Texture> vegetationTexture(Texture::CreateFromFile(texturePathname));

        for (uint32 i = 0; i < VEGETATION_ENTITY_LAYER_NAMES.size(); ++i)
        {
            ScopedPtr<Entity> vegetationLayer(new Entity());
            vegetationLayer->SetName(VEGETATION_ENTITY_LAYER_NAMES[i]);

            ScopedPtr<NMaterial> material(CreateMaterial(VEGETATION_ENTITY_LAYER_NAMES[i], NMaterialName::TEXTURED_OPAQUE_NOCULL));
            material->AddTexture(NMaterialTextureName::TEXTURE_ALBEDO, vegetationTexture);

            ScopedPtr<NMaterial> instanceMaterial(CreateMaterial(VEGETATION_ENTITY_LAYER_NAMES[i], NMaterialName::TEXTURED_OPAQUE_NOCULL));
            instanceMaterial->SetParent(material);

            ScopedPtr<PolygonGroup> geometry(CreatePolygonGroup());
            ScopedPtr<Mesh> ro(new Mesh());
            ro->AddPolygonGroup(geometry, instanceMaterial);
            RenderComponent* rc = new RenderComponent(ro);
            vegetationLayer->AddComponent(rc);
            vegetationLayer->AddComponent(new LodComponent());
            vegetationGeometry->AddNode(vegetationLayer);
        }

        scene->AddNode(vegetationGeometry);
        scene->Update(0.1f);
        scene->SaveScene(customGeometryPathname, false);
    }

    Entity* entity = new Entity();
    entity->SetName(FastName("vegetation"));

    ScopedPtr<VegetationRenderObject> ro(new VegetationRenderObject());
    ro->SetCustomGeometryPath(customGeometryPathname);

    FilePath lightmapPathname = scenePathname;
    lightmapPathname.ReplaceFilename("vegetation.lightmap.tex");
    CreateTextureFiles(lightmapPathname, 128, 128u, PixelFormat::FORMAT_RGBA8888, GetEngineContext()->random->Rand(255));
    ro->SetLightmapAndGenerateDensityMap(lightmapPathname);

    RenderComponent* rc = new RenderComponent(ro);
    entity->AddComponent(rc);

    return entity;
}

Entity* CreateCameraEntity(const FilePath& scenePathname)
{
    Entity* entity = new Entity();
    entity->SetName(FastName("camera"));

    ScopedPtr<Camera> camera(new Camera());
    camera->SetUp(Vector3(0.0f, 0.0f, 1.0f));
    camera->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
    camera->SetTarget(Vector3(0.0f, 0.1f, 0.0f));
    camera->SetupPerspective(90.f, 320.0f / 480.0f, 1.f, 5000.f);
    camera->SetAspect(1.0f);

    entity->AddComponent(new CameraComponent(camera));

    return entity;
}

Entity* CreateLightsEntity(const FilePath& scenePathname)
{
    auto setLightProperties = [](CustomPropertiesComponent* cp, bool enabled)
    {
        KeyedArchive* archieve = cp->GetArchive();
        archieve->SetBool("editor.staticlight.enable", enabled);
        archieve->SetFloat("editor.intensity", 1.f);
        archieve->SetFloat("editor.staticlight.shadowangle", 0.f);
        archieve->SetFloat("editor.staticlight.shadowradius", 0.f);
        archieve->SetInt32("editor.staticlight.shadowsamples", 1);
        archieve->SetFloat("editor.staticlight.falloffcutoff", BeastSystem::DEFAULT_FALLOFFCUTOFF_VALUE);
        archieve->SetFloat("editor.staticlight.falloffexponent", 1.f);
        archieve->SetFloat("editor.staticlight.cone.angle", 90.0f);
        archieve->SetFloat("editor.staticlight.cone.penumbra.angle", 0.0f);
        archieve->SetFloat("editor.staticlight.cone.penumbra.exponent", 1.0f);
    };

    Entity* entity = new Entity();
    entity->SetName(FastName("lights"));

    //dynamic light
    ScopedPtr<Entity> dynamicLightEntity(new Entity());
    dynamicLightEntity->SetName(FastName("dynamicLight"));
    ScopedPtr<Light> dynamicLight(new Light());
    dynamicLight->SetType(Light::TYPE_DIRECTIONAL);
    dynamicLight->SetDynamic(true);
    dynamicLightEntity->AddComponent(new LightComponent(dynamicLight));
    setLightProperties(GetOrCreateCustomProperties(dynamicLightEntity), false);
    entity->AddNode(dynamicLightEntity);

    //not dynamic light
    ScopedPtr<Entity> notDynamicLightEntity(new Entity());
    notDynamicLightEntity->SetName(FastName("notDynamicLight"));
    ScopedPtr<Light> notDynamicLight(new Light());
    notDynamicLight->SetType(Light::TYPE_DIRECTIONAL);
    notDynamicLight->SetDynamic(false);
    notDynamicLightEntity->AddComponent(new LightComponent(notDynamicLight));
    setLightProperties(GetOrCreateCustomProperties(notDynamicLightEntity), true);
    entity->AddNode(notDynamicLightEntity);

    //sky light
    ScopedPtr<Entity> skyLightEntity(new Entity());
    skyLightEntity->SetName(FastName("skyLight"));
    ScopedPtr<Light> skyLight(new Light());
    skyLight->SetType(Light::TYPE_SKY);
    skyLight->SetDynamic(false);
    skyLightEntity->AddComponent(new LightComponent(skyLight));
    setLightProperties(GetOrCreateCustomProperties(skyLightEntity), true);
    entity->AddNode(skyLightEntity);

    return entity;
}

Entity* CreateStaticOcclusionEntity(const FilePath& scenePathname)
{
    Entity* entity = new Entity();
    entity->SetName(FastName("staticOcclusion"));

    StaticOcclusionComponent* so = new StaticOcclusionComponent();
    entity->AddComponent(so);

    return entity;
}
} // namespace Detail

class TextureLoadingGuard::Impl final
{
public:
    Impl(const DAVA::Vector<DAVA::eGPUFamily>& newLoadingOrder)
    {
        gpuLoadingOrder = DAVA::Texture::GetGPULoadingOrder();
        DAVA::Texture::SetGPULoadingOrder(newLoadingOrder);
    }

    ~Impl()
    {
        DAVA::Texture::SetGPULoadingOrder(gpuLoadingOrder);
    }

private:
    DAVA::Vector<DAVA::eGPUFamily> gpuLoadingOrder;
};

TextureLoadingGuard::TextureLoadingGuard(const DAVA::Vector<DAVA::eGPUFamily>& newLoadingOrder)
    : impl(new TextureLoadingGuard::Impl(newLoadingOrder))
{
}

TextureLoadingGuard::~TextureLoadingGuard() = default;

std::unique_ptr<TextureLoadingGuard> CreateTextureGuard(const DAVA::Vector<DAVA::eGPUFamily>& newLoadingOrder)
{
    return std::make_unique<TextureLoadingGuard>(newLoadingOrder);
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::Create(const FilePath& newPath, const String& newName, const String& newTag)
{
    tag = newTag;
    path = newPath;
    name = newName;
    color = GetEngineContext()->random->Rand(255);

    DVASSERT(box.get() == nullptr);
    box = ScopedPtr<Entity>(new Entity());
    box->SetName(FastName(name));

    return *this;
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::SetTextureColor(uint8 newColor)
{
    color = newColor;
    return *this;
}

void SceneBuilder::BoxBuilder::SetupMaterial(NMaterial* material, const String& fileName, const FastName& slotName)
{
    DVASSERT(box.get() != nullptr);
    FilePath texturePath = path;
    texturePath.ReplaceFilename(fileName);
    Detail::CreateTextureFiles(texturePath, 32u, 32u, PixelFormat::FORMAT_RGBA8888, color, tag);

    ScopedPtr<Texture> texture(Texture::CreateFromFile(texturePath));
    material->AddTexture(slotName, texture);

    material->AddFlag(NMaterialFlagName::FLAG_ILLUMINATION_USED, true);
    material->AddFlag(NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER, true);
    material->AddFlag(NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER, true);

    float32 lighmapSize = 64.0f;
    material->AddProperty(NMaterialParamName::PARAM_LIGHTMAP_SIZE, &lighmapSize, rhi::ShaderProp::TYPE_FLOAT1, 1);
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::AddRenderComponent()
{
    DVASSERT(box.get() != nullptr);
    ScopedPtr<NMaterial> material(Detail::CreateMaterial(FastName(name), NMaterialName::TEXTURE_LIGHTMAP_OPAQUE));
    String texName = name + ".tex";
    SetupMaterial(material, texName, NMaterialTextureName::TEXTURE_ALBEDO);

    ScopedPtr<PolygonGroup> geometry(Detail::CreatePolygonGroup());
    ScopedPtr<Mesh> ro(new Mesh());
    ro->AddPolygonGroup(geometry, material);

    RenderComponent* rc = new RenderComponent(ro);
    box->AddComponent(rc);

    return *this;
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::AddGeometry()
{
    DVASSERT(box.get() != nullptr);
    auto addGeometry = [this](int lod, int sw)
    {
        String fileName = Format("%s_%d_%d.tex", name.c_str(), lod, sw);
        ScopedPtr<NMaterial> m(Detail::CreateMaterial(FastName(name), NMaterialName::TEXTURE_LIGHTMAP_OPAQUE));
        SetupMaterial(m, fileName, NMaterialTextureName::TEXTURE_ALBEDO);

        ScopedPtr<PolygonGroup> g(Detail::CreatePolygonGroup());
        ScopedPtr<RenderBatch> batch(new RenderBatch());
        batch->SetMaterial(m);
        batch->SetPolygonGroup(g);
        RenderComponent* rc = box->GetComponent<RenderComponent>();
        DVASSERT(rc != nullptr);
        Mesh* ro = static_cast<Mesh*>(rc->GetRenderObject());
        ro->AddRenderBatch(batch, lod, sw);
    };

    addGeometry(0, 0);
    addGeometry(0, 1);
    addGeometry(1, 0);
    addGeometry(1, 1);

    box->AddComponent(new LodComponent());
    box->AddComponent(new SwitchComponent());

    return *this;
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::AddSlotComponent(const String& slotName, const FilePath& configPath)
{
    DVASSERT(box.get() != nullptr);
    SlotComponent* slotComp = new SlotComponent();
    slotComp->SetSlotName(FastName(slotName));
    slotComp->SetConfigFilePath(configPath);
    box->AddComponent(slotComp);

    return *this;
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::AddRefToOwner()
{
    Detail::CreateR2OCustomProperty(box, path);
    return *this;
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::AddToScene(Scene* scene)
{
    scene->AddNode(box);
    return *this;
}

Entity* SceneBuilder::BoxBuilder::GetBox()
{
    return box.get();
}

SceneBuilder::BoxBuilder& SceneBuilder::BoxBuilder::Reset()
{
    box.reset(nullptr);
    color = GetEngineContext()->random->Rand(255);
    return *this;
}

void CreateTestFolder(const DAVA::FilePath& folder)
{
    ClearTestFolder(folder); // to be sure that we have no any data at project folder that could stay in case of crash or stopping of debugging
    DAVA::FileSystem::Instance()->CreateDirectory(folder, true);
}

void ClearTestFolder(const DAVA::FilePath& folder)
{
    DVASSERT(folder.IsDirectoryPathname());

    DAVA::FileSystem::Instance()->DeleteDirectoryFiles(folder, true);
    DAVA::FileSystem::Instance()->DeleteDirectory(folder, true);
}

bool CreateSlotYaml(const DAVA::FilePath& yamlPath, const DAVA::FilePath& slotContentsPath)
{
    using namespace DAVA;

    ScopedPtr<YamlNode> rootNode(YamlNode::CreateArrayNode(YamlNode::AR_BLOCK_REPRESENTATION));

    YamlNode* slotNode = YamlNode::CreateMapNode(false);
    slotNode->Set(String("Name"), String("Box1"));
    slotNode->Set(String("Type"), String("Box"));
    slotNode->Set(String("Path"), slotContentsPath.GetAbsolutePathname());
    rootNode->Add(slotNode);

    return YamlEmitter::SaveToYamlFile(yamlPath, rootNode);
}

void CreateProjectInfrastructure(const DAVA::FilePath& projectPathname)
{
    ClearTestFolder(projectPathname); // to be sure that we have no any data at project folder that could stay in case of crash or stopping of debugging

    DAVA::FilePath datasourcePath = projectPathname + "DataSource/3d/";
    DAVA::FileSystem::Instance()->CreateDirectory(datasourcePath, true);

    DAVA::FilePath qulityPath = projectPathname + "DataSource/quality.yaml";
    DAVA::FileSystem::Instance()->CopyFile("~res:/ResourceEditor/quality.template.yaml", qulityPath, true);
}

String SceneBuilder::GetSceneRelativePathname(const FilePath& scenePath, const FilePath& dataSourcePath, const String& filename)
{
    FilePath path = scenePath;
    path.ReplaceFilename(filename);
    return path.GetRelativePathname(dataSourcePath);
}

void SceneBuilder::CreateFullScene(const DAVA::FilePath& scenePath, const FilePath& projectPath, DAVA::Scene* scene)
{
    SceneBuilder builder(scenePath, projectPath, scene);
    scene = builder.scene.get();
    builder.AddCamera(SceneBuilder::WITH_REF_TO_OWNER);

    BoxBuilder boxBuilder;

    boxBuilder
    .Create(scenePath, "box", tagDefault)
    .SetTextureColor(32)
    .AddRenderComponent()
    .AddGeometry()
    .AddSlotComponent("slot", projectPath + "/DataSource/Slot.yaml")
    .AddRefToOwner()
    .AddToScene(scene);

    boxBuilder.Reset()
    .Create(scenePath, "box", tagChina)
    .SetTextureColor(64)
    .AddRenderComponent()
    .AddGeometry()
    .AddRefToOwner()
    .AddToScene(scene);

    boxBuilder.Reset()
    .Create(scenePath, "box", tagJapan)
    .SetTextureColor(96)
    .AddRenderComponent()
    .AddGeometry()
    .AddRefToOwner()
    .AddToScene(scene);

    FilePath chinaSlotPath = scenePath;
    chinaSlotPath.ReplaceDirectory(chinaSlotPath.GetDirectory() + chinaSlotDir);

    boxBuilder.Reset()
    .Create(chinaSlotPath, "box_slot", tagDefault)
    .AddRenderComponent()
    .AddRefToOwner();

    FilePath defaultSlotPath = scenePath;
    defaultSlotPath.ReplaceDirectory(defaultSlotPath.GetDirectory() + defaultSlotDir);

    boxBuilder.Reset()
    .Create(defaultSlotPath, "box_slot", tagDefault)
    .AddRenderComponent()
    .AddRefToOwner();

    builder.AddLandscape(SceneBuilder::WITH_REF_TO_OWNER);
    builder.AddWater(SceneBuilder::WITH_REF_TO_OWNER);
    builder.AddSky(SceneBuilder::WITH_REF_TO_OWNER);
    builder.AddVegetation(SceneBuilder::WITH_REF_TO_OWNER);
    builder.AddLights(SceneBuilder::WITH_REF_TO_OWNER);
    builder.AddStaticOcclusion(SceneBuilder::WITH_REF_TO_OWNER);
    builder.AddEntityWithTestedComponents(SceneBuilder::WITH_REF_TO_OWNER);
}

SceneBuilder::SceneBuilder(const FilePath& scenePathname_, const FilePath& projectPathname_, Scene* scene_)
    : scenePathname(scenePathname_)
    , projectPathname(projectPathname_)
    , scene(SafeRetain(scene_))
{
    FileSystem* fs = GetEngineContext()->fileSystem;
    fs->CreateDirectory(scenePathname.GetDirectory(), true);
    fs->CreateDirectory(scenePathname.GetDirectory() + chinaSlotDir);
    fs->CreateDirectory(scenePathname.GetDirectory() + defaultSlotDir);

    DAVA::FilePath chineseSlotPath = projectPathname + "DataSource/Slot.china.yaml";
    CreateSlotYaml(chineseSlotPath, scenePathname.GetDirectory() + chinaSlotDir + "box_slot.sc2");

    DAVA::FilePath defaultSlotPath = projectPathname + "DataSource/Slot.yaml";
    CreateSlotYaml(defaultSlotPath, scenePathname.GetDirectory() + defaultSlotDir + "box_slot.sc2");

    if (!scene)
    {
        scene.reset(new Scene);
    }
}

SceneBuilder::~SceneBuilder()
{
    scene->Update(0.1f);
    scene->SaveScene(scenePathname, false);
}

Entity* SceneBuilder::AddCamera(R2OMode mode)
{
    ScopedPtr<Entity> cameraEntity(Detail::CreateCameraEntity(scenePathname));
    scene->AddNode(cameraEntity);
    Camera* camera = GetCamera(cameraEntity);
    scene->SetCurrentCamera(camera);

    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(cameraEntity);
    }

    return cameraEntity;
}

Entity* SceneBuilder::AddBox(R2OMode mode)
{
    BoxBuilder boxBuilder;
    boxBuilder.Create(scenePathname, "box", tagDefault)
    .AddRenderComponent()
    .AddGeometry();

    if (mode == SceneBuilder::WITH_REF_TO_OWNER)
    {
        boxBuilder.AddRefToOwner();
    }

    boxBuilder.AddToScene(scene);

    return boxBuilder.GetBox();
}

Entity* SceneBuilder::AddLandscape(R2OMode mode)
{
    ScopedPtr<Entity> landscape(Detail::CreateLandscapeEnity(scenePathname));
    scene->AddNode(landscape);

    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(landscape);
    }

    return landscape;
}

Entity* SceneBuilder::AddWater(R2OMode mode)
{
    ScopedPtr<Entity> water(Detail::CreateWaterEntity(scenePathname));
    scene->AddNode(water);

    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(water);
    }

    return water;
}

Entity* SceneBuilder::AddSky(R2OMode mode)
{
    ScopedPtr<Entity> sky(Detail::CreateSkyEntity(scenePathname));
    scene->AddNode(sky);

    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(sky);
    }

    return sky;
}

Entity* SceneBuilder::AddVegetation(R2OMode mode)
{
    ScopedPtr<Entity> vegetation(Detail::CreateVegetationEntity(scenePathname));
    scene->AddNode(vegetation);

    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(vegetation);
    }

    return vegetation;
}

Entity* SceneBuilder::AddLights(R2OMode mode)
{
    ScopedPtr<Entity> lights(Detail::CreateLightsEntity(scenePathname));
    scene->AddNode(lights);

    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(lights);
    }

    return lights;
}

Entity* SceneBuilder::AddStaticOcclusion(R2OMode mode)
{
    ScopedPtr<Entity> occlusion(Detail::CreateStaticOcclusionEntity(scenePathname));
    scene->AddNode(occlusion);

    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(occlusion);
    }

    return occlusion;
}

Entity* SceneBuilder::AddEntityWithTestedComponents(R2OMode mode)
{
    ScopedPtr<Entity> entity(new Entity);
    entity->SetName(FastName("entityWithTestedComponents"));

    TextComponent* text = new TextComponent();
    text->SetText(L"text");
    text->SetSize(10);
    entity->AddComponent(text);

    text = new TextComponent();
    text->SetText(L"super text");
    text->SetSize(20);
    entity->AddComponent(text);

    entity->AddComponent(new StaticOcclusionDebugDrawComponent());
    entity->AddComponent(new WaypointComponent());
    entity->AddComponent(new EdgeComponent());

    scene->AddNode(entity);
    if (mode == WITH_REF_TO_OWNER)
    {
        AddR2O(entity);
    }

    return entity;
}

void SceneBuilder::AddR2O(Entity* entity)
{
    Detail::CreateR2OCustomProperty(entity, scenePathname);
}
}
