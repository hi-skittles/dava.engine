#include "MaterialsTest.h"

#include <Scene3D/Components/TransformComponent.h>

const FastName MaterialsTest::LIGHT_ENTITY = FastName("Light");
const FastName MaterialsTest::CAMERA_ENTITY = FastName("OrthoCamera");
const FastName MaterialsTest::PLANE_ENTITY = FastName("plane");
const FastName MaterialsTest::MATERIALS_ENTITY = FastName("Materials");

const String MaterialsTest::TEST_NAME = "MaterialsTest";
const String MaterialsTest::SPHERICAL_LIT_MATERIAL = "SphericalLit";
const String MaterialsTest::SKINNED_MATERIAL = "Skinning";
const String MaterialsTest::LIGHTMAP_MATERIAL = "Lightmap";

const uint32 MaterialsTest::FRAMES_PER_MATERIAL_TEST = 60;

MaterialsTest::MaterialsTest(const TestParams& testParams)
    : BaseTest(TEST_NAME, testParams)
    , currentTestStartFrame(0)
    , currentTestStartTimeMs(0)
    , currentMaterialIndex(0)
{
}

void MaterialsTest::LoadResources()
{
    BaseTest::LoadResources();

    ScopedPtr<Scene> materialsScene(new Scene());

    SceneFileV2::eError error = materialsScene->LoadScene(FilePath("~res:/3d/Maps/" + GetParams().scenePath));
    DVASSERT(error == SceneFileV2::eError::ERROR_NO_ERROR, ("can't load scene " + GetParams().scenePath).c_str());

    Entity* materialsEntity = materialsScene->FindByName(MATERIALS_ENTITY);

    for (int32 i = 0; i < materialsEntity->GetChildrenCount(); i++)
    {
        RenderComponent* renderComponent = materialsEntity->GetChild(i)->GetComponent<RenderComponent>();
        NMaterial* material = renderComponent->GetRenderObject()->GetRenderBatch(0)->GetMaterial()->Clone();

        materials.push_back(material);
    }

    Entity* planeEntity = materialsScene->FindByName(PLANE_ENTITY);

    for (int32 i = 0; i < 11; i++)
    {
        Entity* clone = planeEntity->Clone();
        TransformComponent* tc = clone->GetComponent<TransformComponent>();

        tc->SetLocalTranslation(tc->GetLocalTransform().GetTranslation() + Vector3(0.0f + i * 10.0f, 0.0f, 0.0f));
        GetScene()->AddNode(clone);

        planes.push_back(clone);
        spoPlanes.push_back(CreateSpeedTreeEntity(clone));
        skinnedPlanes.push_back(CreateSkinnedEntity(clone));
        lightmapMaterialPlanes.push_back(CreateEntityForLightmapMaterial(clone));
    }

    Entity* light = materialsScene->FindByName(LIGHT_ENTITY);
    GetScene()->AddNode(light);

    Entity* camera = materialsScene->FindByName(CAMERA_ENTITY);
    CameraComponent* cameraComponent = camera->GetComponent<CameraComponent>();
    GetScene()->SetCurrentCamera(cameraComponent->GetCamera());
}

void MaterialsTest::UnloadResources()
{
    for (NMaterial* material : materials)
    {
        SafeRelease(material);
    }
    materials.clear();

    for (Entity* child : planes)
    {
        SafeRelease(child);
    }
    planes.clear();

    for (Entity* child : spoPlanes)
    {
        SafeRelease(child);
    }
    spoPlanes.clear();

    for (Entity* child : skinnedPlanes)
    {
        SafeRelease(child);
    }
    skinnedPlanes.clear();

    for (Entity* child : lightmapMaterialPlanes)
    {
        SafeRelease(child);
    }
    lightmapMaterialPlanes.clear();

    BaseTest::UnloadResources();
}

void MaterialsTest::BeginFrame()
{
    BaseTest::BeginFrame();

    // material test finished
    if (GetTestFrameNumber() - currentTestStartFrame == FRAMES_PER_MATERIAL_TEST)
    {
        float32 testTime = static_cast<float32>(SystemTimer::GetFrameTimestampMs() - currentTestStartTimeMs) / 1000.f;
        materialTestsElapsedTime.push_back(testTime);

        NMaterial* material = materials[currentMaterialIndex]->GetParent();

        if (material->GetMaterialName().find(SPHERICAL_LIT_MATERIAL) != String::npos ||
            material->GetMaterialName().find(SKINNED_MATERIAL) != String::npos ||
            material->GetMaterialName().find(LIGHTMAP_MATERIAL) != String::npos)
        {
            ReplacePlanes(planes);
        }

        currentMaterialIndex++;
    }

    // material test started
    if (GetTestFrameNumber() % FRAMES_PER_MATERIAL_TEST == 0)
    {
        size_t materialsCount = materials.size();

        if (currentMaterialIndex < materialsCount)
        {
            NMaterial* currentMaterial = materials[currentMaterialIndex]->GetParent();

            if (currentMaterial->GetMaterialName().find(SPHERICAL_LIT_MATERIAL) != String::npos)
            {
                ReplacePlanes(spoPlanes);
            }

            if (currentMaterial->GetMaterialName().find(SKINNED_MATERIAL) != String::npos)
            {
                ReplacePlanes(skinnedPlanes);
            }

            if (currentMaterial->GetMaterialName().find(LIGHTMAP_MATERIAL) != String::npos)
            {
                ReplacePlanes(lightmapMaterialPlanes);
            }

            List<Entity*> children;
            GetScene()->FindNodesByNamePart(PLANE_ENTITY.c_str(), children);

            for (Entity* child : children)
            {
                RenderComponent* renderComponent = child->GetComponent<RenderComponent>();
                renderComponent->GetRenderObject()->GetRenderBatch(0)->SetMaterial(materials[currentMaterialIndex]);
            }
        }

        currentTestStartTimeMs = SystemTimer::GetFrameTimestampMs();
        currentTestStartFrame = GetTestFrameNumber();
    }
}

bool MaterialsTest::IsFinished() const
{
    return static_cast<size_t>(GetTestFrameNumber()) > materials.size() * FRAMES_PER_MATERIAL_TEST;
}

void MaterialsTest::PrintStatistic(const Vector<BaseTest::FrameInfo>& frames)
{
    BaseTest::PrintStatistic(frames);

    for (uint32 i = 0; i < materials.size(); i++)
    {
        String materialName = "MaterialSubtestName:" + String(materials[i]->GetParent()->GetMaterialName().c_str());
        Logger::Info(materialName.c_str());

        float32 materialSubtestTime = 0.0f;
        float32 materialSubtestElapsedTime = materialTestsElapsedTime[i];

        for (uint32 j = FRAMES_PER_MATERIAL_TEST * i; j < FRAMES_PER_MATERIAL_TEST * (i + 1); j++)
        {
            materialSubtestTime += frames[j].delta;

            Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                         TeamcityPerformanceTestsOutput::MATERIAL_FRAME_DELTA,
                         DAVA::Format("%f", frames[j].delta))
                         .c_str());
        }

        Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                     TeamcityPerformanceTestsOutput::MATERIAL_TEST_TIME,
                     DAVA::Format("%f", materialSubtestTime))
                     .c_str());

        Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                     TeamcityPerformanceTestsOutput::MATERIAL_ELAPSED_TEST_TIME,
                     DAVA::Format("%f", materialSubtestElapsedTime))
                     .c_str());
    }
}

Entity* MaterialsTest::CreateSpeedTreeEntity(Entity* entity)
{
    Entity* spoEntity = entity->Clone();
    RenderComponent* spoRenderComponent = spoEntity->GetComponent<RenderComponent>();
    RenderObject* renderObject = spoRenderComponent->GetRenderObject();
    PolygonGroup* polygonGroup = renderObject->GetRenderBatch(0)->GetPolygonGroup();

    ScopedPtr<RenderBatch> spoRenderBatch(new RenderBatch());
    ScopedPtr<PolygonGroup> spoPolygonGroup(new PolygonGroup());
    ScopedPtr<SpeedTreeObject> spoRenderObject(new SpeedTreeObject());

    int32 vertexCount = polygonGroup->GetVertexCount();
    int32 indexCount = polygonGroup->GetIndexCount();

    spoPolygonGroup->AllocateData(EVF_VERTEX | EVF_COLOR | EVF_TEXCOORD0 | EVF_NORMAL, vertexCount, indexCount);

    Vector3 v;
    Vector2 t;

    int32 index;

    for (int32 i = 0; i < vertexCount; i++)
    {
        polygonGroup->GetCoord(i, v);
        spoPolygonGroup->SetCoord(i, v);

        polygonGroup->GetNormal(i, v);
        spoPolygonGroup->SetNormal(i, v);

        polygonGroup->GetTexcoord(0, i, t);
        spoPolygonGroup->SetTexcoord(0, i, t);

        spoPolygonGroup->SetColor(i, Color(1.0f, 0.0f, 1.0f, 1.0f).GetRGBA());
    }

    for (int32 i = 0; i < indexCount; i++)
    {
        polygonGroup->GetIndex(i, index);
        spoPolygonGroup->SetIndex(i, index);
    }

    spoPolygonGroup->RecalcAABBox();
    spoPolygonGroup->BuildBuffers();

    // fix for calculate normals in ShpericalLit shader
    spoPolygonGroup->aabbox.max.z = 1.0f;

    spoRenderBatch->SetPolygonGroup(spoPolygonGroup);
    spoRenderBatch->SetMaterial(ScopedPtr<NMaterial>(renderObject->GetRenderBatch(0)->GetMaterial()->Clone()));
    spoRenderObject->AddRenderBatch(spoRenderBatch);
    spoRenderComponent->SetRenderObject(spoRenderObject);

    DAVA::Array<DAVA::float32, SpeedTreeObject::HARMONICS_BUFFER_CAPACITY> harmonics = {};
    harmonics[0] = harmonics[1] = harmonics[2] = 1.f / 0.564188f; //fake SH value to make original object color
    spoRenderObject->SetSphericalHarmonics(harmonics);

    if (GetSpeedTreeComponent(spoEntity) == nullptr)
        spoEntity->AddComponent(new SpeedTreeComponent());

    return spoEntity;
}

Entity* MaterialsTest::CreateSkinnedEntity(Entity* sourceEntity)
{
    Entity* skinnedEntity = sourceEntity->Clone();

    ScopedPtr<Entity> entityHierarhy(new Entity());
    entityHierarhy->AddNode(ScopedPtr<Entity>(sourceEntity->Clone()));

    Vector<SkeletonComponent::Joint> joints;

    RenderComponent* renderComponent = skinnedEntity->GetComponent<RenderComponent>();
    renderComponent->SetRenderObject(ScopedPtr<RenderObject>(MeshUtils::CreateHardSkinnedMesh(entityHierarhy, joints)));

    SkeletonComponent* skeleton = new SkeletonComponent();
    skinnedEntity->AddComponent(skeleton);

    skeleton->SetJoints(joints);

    return skinnedEntity;
}

Entity* MaterialsTest::CreateEntityForLightmapMaterial(DAVA::Entity* entity)
{
    Entity* lightmapEntity = entity->Clone();
    RenderComponent* lightmapRenderComponent = lightmapEntity->GetComponent<RenderComponent>();
    RenderObject* renderObject = lightmapRenderComponent->GetRenderObject();
    PolygonGroup* polygonGroup = renderObject->GetRenderBatch(0)->GetPolygonGroup();

    ScopedPtr<PolygonGroup> lightmapPolygonGroup(new PolygonGroup());

    int32 vertexCount = polygonGroup->GetVertexCount();
    int32 indexCount = polygonGroup->GetIndexCount();

    lightmapPolygonGroup->AllocateData(EVF_VERTEX | EVF_NORMAL | EVF_TEXCOORD0 | EVF_TEXCOORD1, vertexCount, indexCount);

    Vector3 v;
    Vector2 t;

    int32 index;

    for (int32 i = 0; i < vertexCount; i++)
    {
        polygonGroup->GetCoord(i, v);
        lightmapPolygonGroup->SetCoord(i, v);

        polygonGroup->GetNormal(i, v);
        lightmapPolygonGroup->SetNormal(i, v);

        polygonGroup->GetTexcoord(0, i, t);
        lightmapPolygonGroup->SetTexcoord(0, i, t);
        lightmapPolygonGroup->SetTexcoord(1, i, t);
    }

    for (int32 i = 0; i < indexCount; i++)
    {
        polygonGroup->GetIndex(i, index);
        lightmapPolygonGroup->SetIndex(i, index);
    }

    lightmapPolygonGroup->RecalcAABBox();
    lightmapPolygonGroup->BuildBuffers();

    renderObject->GetRenderBatch(0)->SetPolygonGroup(lightmapPolygonGroup);

    return lightmapEntity;
}

void MaterialsTest::ReplacePlanes(const Vector<Entity*>& planes)
{
    List<Entity*> children;
    GetScene()->FindNodesByNamePart(PLANE_ENTITY.c_str(), children);

    for (auto* child : children)
    {
        GetScene()->RemoveNode(child);
    }

    for (auto* child : planes)
    {
        GetScene()->AddNode(child);
    }
}

const String& MaterialsTest::GetSceneName() const
{
    return GetTestName();
}
