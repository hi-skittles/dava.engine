#include "DeveloperTools.h"
#include "Main/mainwindow.h"
#include "Commands2/EntityAddCommand.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"

#include "Qt/ImageSplitterDialog/ImageSplitterDialogNormal.h"

#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Systems/StaticOcclusionSystem.h"
#include "Scene3D/Systems/LandscapeSystem.h"

#include "Render/Highlevel/SkinnedMesh.h"

#include "Classes/Selection/Selection.h"

#include <QInputDialog>

namespace DeveloperToolsDetails
{
SceneEditor2* GetActiveScene()
{
    SceneData* data = REGlobal::GetActiveDataNode<SceneData>();
    if (data != nullptr)
    {
        return data->GetScene().Get();
    }
    return nullptr;
}
}

DeveloperTools::DeveloperTools(QWidget* parent)
    : QObject(parent)
{
}

void DeveloperTools::OnDebugFunctionsGridCopy()
{
    using namespace DAVA;

    SceneEditor2* currentScene = DeveloperToolsDetails::GetActiveScene();
    float32 z = 0;
    const float32 xshift = 10.0;
    const float32 yshift = 10.0;
    const float32 zshift = 0.0;

    FastName inGlossinessName("inGlossiness");
    FastName inSpecularityName("inSpecularity");

    const SelectableGroup& selection = Selection::GetSelection();
    if (selection.GetSize() == 1)
    {
        Entity* entity = selection.GetContent().front().AsEntity();

        const Matrix4& matrix = entity->GetLocalTransform();

        for (uint32 x = 0; x < 10; ++x)
        {
            for (uint32 y = 0; y < 10; ++y)
            {
                Matrix4 translation;
                translation.BuildTranslation(Vector3(x * xshift, y * yshift, z * zshift));

                Matrix4 newMatrix = matrix * translation;
                Entity* clonedEntity = entity->Clone();
                clonedEntity->SetLocalTransform(newMatrix);

                RenderObject* renderObject = GetRenderObject(clonedEntity);
                NMaterial* material = renderObject->GetRenderBatch(0)->GetMaterial();
                float32 inGlossiness = (float32)x / 9.0;
                float32 inSpecularity = (float32)y / 9.0;

                if (material->HasLocalProperty(inGlossinessName))
                    material->SetPropertyValue(inGlossinessName, &inGlossiness);
                else
                    material->AddProperty(inGlossinessName, &inGlossiness, rhi::ShaderProp::TYPE_FLOAT1);

                if (material->HasLocalProperty(inSpecularityName))
                    material->SetPropertyValue(inSpecularityName, &inSpecularity);
                else
                    material->AddProperty(inSpecularityName, &inSpecularity, rhi::ShaderProp::TYPE_FLOAT1);

                StaticOcclusionSystem* sosystem = currentScene->staticOcclusionSystem;
                DVASSERT(sosystem);
                sosystem->InvalidateOcclusionIndicesRecursively(clonedEntity);

                currentScene->AddNode(clonedEntity);
            }
        }
    }
}

void DeveloperTools::OnDebugCreateTestHardSkinnedObject()
{
    using namespace DAVA;

    SceneEditor2* currentScene = DeveloperToolsDetails::GetActiveScene();
    if (!currentScene)
        return;

    ScopedPtr<Entity> entity(new Entity());
    entity->SetName(FastName("TestHardSkinned"));

    int boxesCount = 4;
    Vector3 boxes[] = { Vector3(0, 0, 0), Vector3(0, 0, 10), Vector3(2, 0, 5), Vector3(-2, 0, 5) };

    AABBox3 jointBox(Vector3(-1, -1, -1), Vector3(1, 1, 1));
    SkeletonComponent* component = new SkeletonComponent();

    Vector<SkeletonComponent::Joint> joints;
    joints.resize(4);

    joints[0].parentIndex = SkeletonComponent::INVALID_JOINT_INDEX;
    joints[0].uid = FastName("root0");

    joints[1].parentIndex = 0;
    joints[1].uid = FastName("root0.bone0");

    joints[2].parentIndex = 1;
    joints[2].uid = FastName("root0.bone0.bone0");

    joints[3].parentIndex = 1;
    joints[3].uid = FastName("root0.bone0.bone1");

    for (int32 i = 0; i < 4; ++i)
    {
        joints[i].name = joints[i].uid;
        joints[i].bindTransform = Matrix4::MakeTranslation(boxes[i]);
        joints[i].bindTransform.GetInverse(joints[i].bindTransformInv);
        joints[i].bbox = AABBox3(jointBox.min + boxes[i], jointBox.max + boxes[i]);
    }

    component->SetJoints(joints);
    entity->AddComponent(component);

    ScopedPtr<PolygonGroup> polygonGroup(new PolygonGroup());
    polygonGroup->SetPrimitiveType(rhi::PRIMITIVE_LINELIST);
    polygonGroup->AllocateData(EVF_VERTEX | EVF_HARD_JOINTINDEX, boxesCount * 8, boxesCount * 24);
    for (int32 i = 0; i < boxesCount; i++)
    {
        polygonGroup->SetCoord(i * 8 + 0, boxes[i] + Vector3(jointBox.min.x, jointBox.min.y, jointBox.min.z));
        polygonGroup->SetCoord(i * 8 + 1, boxes[i] + Vector3(jointBox.min.x, jointBox.max.y, jointBox.min.z));
        polygonGroup->SetCoord(i * 8 + 2, boxes[i] + Vector3(jointBox.max.x, jointBox.max.y, jointBox.min.z));
        polygonGroup->SetCoord(i * 8 + 3, boxes[i] + Vector3(jointBox.max.x, jointBox.min.y, jointBox.min.z));
        polygonGroup->SetCoord(i * 8 + 4, boxes[i] + Vector3(jointBox.min.x, jointBox.min.y, jointBox.max.z));
        polygonGroup->SetCoord(i * 8 + 5, boxes[i] + Vector3(jointBox.min.x, jointBox.max.y, jointBox.max.z));
        polygonGroup->SetCoord(i * 8 + 6, boxes[i] + Vector3(jointBox.max.x, jointBox.max.y, jointBox.max.z));
        polygonGroup->SetCoord(i * 8 + 7, boxes[i] + Vector3(jointBox.max.x, jointBox.min.y, jointBox.max.z));
        for (int32 v = 0; v < 8; v++)
        {
            polygonGroup->SetHardJointIndex(i * 8 + v, i);
        }

        polygonGroup->SetIndex(i * 24 + 0, i * 8 + 0);
        polygonGroup->SetIndex(i * 24 + 1, i * 8 + 1);
        polygonGroup->SetIndex(i * 24 + 2, i * 8 + 1);
        polygonGroup->SetIndex(i * 24 + 3, i * 8 + 2);
        polygonGroup->SetIndex(i * 24 + 4, i * 8 + 2);
        polygonGroup->SetIndex(i * 24 + 5, i * 8 + 3);
        polygonGroup->SetIndex(i * 24 + 6, i * 8 + 3);
        polygonGroup->SetIndex(i * 24 + 7, i * 8 + 0);

        polygonGroup->SetIndex(i * 24 + 8, i * 8 + 0);
        polygonGroup->SetIndex(i * 24 + 9, i * 8 + 4);
        polygonGroup->SetIndex(i * 24 + 10, i * 8 + 1);
        polygonGroup->SetIndex(i * 24 + 11, i * 8 + 5);
        polygonGroup->SetIndex(i * 24 + 12, i * 8 + 2);
        polygonGroup->SetIndex(i * 24 + 13, i * 8 + 6);
        polygonGroup->SetIndex(i * 24 + 14, i * 8 + 3);
        polygonGroup->SetIndex(i * 24 + 15, i * 8 + 7);
        polygonGroup->SetIndex(i * 24 + 16, i * 8 + 4);
        polygonGroup->SetIndex(i * 24 + 17, i * 8 + 5);
        polygonGroup->SetIndex(i * 24 + 18, i * 8 + 5);
        polygonGroup->SetIndex(i * 24 + 19, i * 8 + 6);
        polygonGroup->SetIndex(i * 24 + 20, i * 8 + 6);
        polygonGroup->SetIndex(i * 24 + 21, i * 8 + 7);
        polygonGroup->SetIndex(i * 24 + 22, i * 8 + 7);
        polygonGroup->SetIndex(i * 24 + 23, i * 8 + 4);
    }

    polygonGroup->BuildBuffers();

    ScopedPtr<NMaterial> material(new NMaterial());
    material->SetMaterialName(FastName("DebugSkeleton"));
    material->SetFXName(NMaterialName::DECAL_OPAQUE);
    material->AddFlag(NMaterialFlagName::FLAG_HARD_SKINNING, 1);

    ScopedPtr<RenderBatch> renderBatch(new RenderBatch());
    renderBatch->SetMaterial(material);
    renderBatch->SetPolygonGroup(polygonGroup);

    ScopedPtr<SkinnedMesh> skinnedMesh(new SkinnedMesh());
    skinnedMesh->AddRenderBatch(renderBatch);

    RenderComponent* renderComponent = new RenderComponent();
    renderComponent->SetRenderObject(skinnedMesh);
    entity->AddComponent(renderComponent);

    currentScene->Exec(std::unique_ptr<DAVA::Command>(new EntityAddCommand(entity, currentScene)));
}

void DeveloperTools::OnDebugCreateTestSoftSkinnedObject()
{
    using namespace DAVA;

    SceneEditor2* currentScene = DeveloperToolsDetails::GetActiveScene();
    if (!currentScene)
        return;

    ScopedPtr<Entity> entity(new Entity());
    entity->SetName(FastName("TestSoftSkinned"));

    //////////////////////////////////////////////////////////////////////////

    SkeletonComponent* component = new SkeletonComponent();
    Vector<SkeletonComponent::Joint> joints;
    joints.resize(5);

    AABBox3 jointBBox = AABBox3(Vector3(0.f, 0.f, 0.f), 1.f);

    joints[0].uid = FastName("root");
    joints[0].bindTransform = Matrix4::IDENTITY;

    joints[1].uid = FastName("corner00");
    joints[1].bindTransform = Matrix4::MakeTranslation(Vector3(-10.f, -10.f, 0.f));

    joints[2].uid = FastName("corner01");
    joints[2].bindTransform = Matrix4::MakeTranslation(Vector3(-10.f, 10.f, 0.f));

    joints[3].uid = FastName("corner10");
    joints[3].bindTransform = Matrix4::MakeTranslation(Vector3(10.f, -10.f, 0.f));

    joints[4].uid = FastName("corner11");
    joints[4].bindTransform = Matrix4::MakeTranslation(Vector3(10.f, 10.f, 0.f));

    for (int32 j = 0; j < 5; ++j)
    {
        joints[j].parentIndex = (j == 0) ? SkeletonComponent::INVALID_JOINT_INDEX : 0;
        joints[j].name = joints[j].uid;
        joints[j].bbox = jointBBox;
        joints[j].bindTransform.GetInverse(joints[j].bindTransformInv);
    }

    component->SetJoints(joints);
    entity->AddComponent(component);

    //////////////////////////////////////////////////////////////////////////

    ScopedPtr<PolygonGroup> polygonGroup(new PolygonGroup());
    polygonGroup->AllocateData(EVF_VERTEX | EVF_TEXCOORD0 | EVF_NORMAL | EVF_JOINTINDEX | EVF_JOINTWEIGHT, 9, 8 * 3);

    for (int32 v = 0; v < 9; ++v)
    {
        for (int32 j = 0; j < 4; ++j)
        {
            polygonGroup->SetJointIndex(v, j, 0);
            polygonGroup->SetJointWeight(v, j, 0.f);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    polygonGroup->SetIndex(0, 1);
    polygonGroup->SetIndex(1, 0);
    polygonGroup->SetIndex(2, 4);

    polygonGroup->SetIndex(3, 4);
    polygonGroup->SetIndex(4, 0);
    polygonGroup->SetIndex(5, 3);

    polygonGroup->SetIndex(6, 1);
    polygonGroup->SetIndex(7, 4);
    polygonGroup->SetIndex(8, 2);

    polygonGroup->SetIndex(9, 2);
    polygonGroup->SetIndex(10, 4);
    polygonGroup->SetIndex(11, 5);

    polygonGroup->SetIndex(12, 3);
    polygonGroup->SetIndex(13, 6);
    polygonGroup->SetIndex(14, 4);

    polygonGroup->SetIndex(15, 4);
    polygonGroup->SetIndex(16, 6);
    polygonGroup->SetIndex(17, 7);

    polygonGroup->SetIndex(18, 5);
    polygonGroup->SetIndex(19, 4);
    polygonGroup->SetIndex(20, 8);

    polygonGroup->SetIndex(21, 8);
    polygonGroup->SetIndex(22, 4);
    polygonGroup->SetIndex(23, 7);

    //////////////////////////////////////////////////////////////////////////

    polygonGroup->SetCoord(0, Vector3(-10.f, -10.f, 0.f));
    polygonGroup->SetCoord(1, Vector3(-10.f, 0.f, 0.f));
    polygonGroup->SetCoord(2, Vector3(-10.f, 10.f, 0.f));

    polygonGroup->SetCoord(3, Vector3(0.f, -10.f, 0.f));
    polygonGroup->SetCoord(4, Vector3(0.f, 0.f, 0.f));
    polygonGroup->SetCoord(5, Vector3(0.f, 10.f, 0.f));

    polygonGroup->SetCoord(6, Vector3(10.f, -10.f, 0.f));
    polygonGroup->SetCoord(7, Vector3(10.f, 0.f, 0.f));
    polygonGroup->SetCoord(8, Vector3(10.f, 10.f, 0.f));

    //////////////////////////////////////////////////////////////////////////

    polygonGroup->SetTexcoord(0, 0, Vector2(0.f, 0.0f));
    polygonGroup->SetTexcoord(0, 1, Vector2(0.f, 0.5f));
    polygonGroup->SetTexcoord(0, 2, Vector2(0.f, 1.0f));

    polygonGroup->SetTexcoord(0, 3, Vector2(0.5f, 0.0f));
    polygonGroup->SetTexcoord(0, 4, Vector2(0.5f, 0.5f));
    polygonGroup->SetTexcoord(0, 5, Vector2(0.5f, 1.0f));

    polygonGroup->SetTexcoord(0, 6, Vector2(1.f, 0.0f));
    polygonGroup->SetTexcoord(0, 7, Vector2(1.f, 0.5f));
    polygonGroup->SetTexcoord(0, 8, Vector2(1.f, 1.0f));

    //////////////////////////////////////////////////////////////////////////

    for (int32 v = 0; v < 9; ++v)
    {
        polygonGroup->SetNormal(v, Vector3(0.f, 0.f, 1.f));
    }

    //////////////////////////////////////////////////////////////////////////

    polygonGroup->SetJointIndex(0, 0, 1);
    polygonGroup->SetJointWeight(0, 0, 1.f);

    polygonGroup->SetJointIndex(1, 0, 1);
    polygonGroup->SetJointIndex(1, 1, 2);
    polygonGroup->SetJointWeight(1, 0, 0.5f);
    polygonGroup->SetJointWeight(1, 1, 0.5f);

    polygonGroup->SetJointIndex(2, 0, 2);
    polygonGroup->SetJointWeight(2, 0, 1.f);

    polygonGroup->SetJointIndex(3, 0, 1);
    polygonGroup->SetJointIndex(3, 1, 3);
    polygonGroup->SetJointWeight(3, 0, 0.5f);
    polygonGroup->SetJointWeight(3, 1, 0.5f);

    polygonGroup->SetJointIndex(4, 0, 0);
    polygonGroup->SetJointWeight(4, 0, 1.f);

    polygonGroup->SetJointIndex(5, 0, 2);
    polygonGroup->SetJointIndex(5, 1, 4);
    polygonGroup->SetJointWeight(5, 0, 0.5f);
    polygonGroup->SetJointWeight(5, 1, 0.5f);

    polygonGroup->SetJointIndex(6, 0, 3);
    polygonGroup->SetJointWeight(6, 0, 1.f);

    polygonGroup->SetJointIndex(7, 0, 3);
    polygonGroup->SetJointIndex(7, 1, 4);
    polygonGroup->SetJointWeight(7, 0, 0.5f);
    polygonGroup->SetJointWeight(7, 1, 0.5f);

    polygonGroup->SetJointIndex(8, 0, 4);
    polygonGroup->SetJointWeight(8, 0, 1.f);

    //////////////////////////////////////////////////////////////////////////

    MeshUtils::RebuildMeshTangentSpace(polygonGroup);
    polygonGroup->BuildBuffers();

    ScopedPtr<NMaterial> material(new NMaterial());
    material->SetMaterialName(FastName("DebugSkeleton"));
    material->SetFXName(FastName("~res:/Materials/NormalizedBlinnPhongPerVertex.Opaque.material"));
    material->AddFlag(NMaterialFlagName::FLAG_SOFT_SKINNING, 4);

    ScopedPtr<RenderBatch> renderBatch(new RenderBatch());
    renderBatch->SetMaterial(material);
    renderBatch->SetPolygonGroup(polygonGroup);

    ScopedPtr<SkinnedMesh> skinnedMesh(new SkinnedMesh());
    skinnedMesh->AddRenderBatch(renderBatch);

    RenderComponent* renderComponent = new RenderComponent();
    renderComponent->SetRenderObject(skinnedMesh);
    entity->AddComponent(renderComponent);

    currentScene->Exec(std::unique_ptr<DAVA::Command>(new EntityAddCommand(entity, currentScene)));
}

void DeveloperTools::OnImageSplitterNormals()
{
    ImageSplitterDialogNormal dlg(GetParentWidget());
    dlg.exec();
}

void DeveloperTools::OnReplaceTextureMipmap()
{
    using namespace DAVA;

    QStringList items = QStringList()
    << QString(NMaterialTextureName::TEXTURE_ALBEDO.c_str())
    << QString(NMaterialTextureName::TEXTURE_LIGHTMAP.c_str())
    << QString(NMaterialTextureName::TEXTURE_DETAIL.c_str())
    << QString(NMaterialTextureName::TEXTURE_NORMAL.c_str());

    bool isOk;
    QString item = QInputDialog::getItem(GetParentWidget(), "Replace mipmaps", "Textures:", items, 0, true, &isOk);
    if (isOk)
    {
        SceneEditor2* currentScene = DeveloperToolsDetails::GetActiveScene();
        MipMapReplacer::ReplaceMipMaps(currentScene, FastName(item.toStdString().c_str()));
    }
}

void DeveloperTools::OnToggleLandscapeInstancing()
{
    using namespace DAVA;

    SceneEditor2* currentScene = DeveloperToolsDetails::GetActiveScene();
    for (Landscape* l : currentScene->landscapeSystem->GetLandscapeObjects())
    {
        l->SetUseInstancing(!l->IsUseInstancing());
        Logger::FrameworkDebug("Landscape uses instancing: %s", l->IsUseInstancing() ? "true" : "false");
    }
}

QWidget* DeveloperTools::GetParentWidget()
{
    QWidget* parentWidget = qobject_cast<QWidget*>(parent());
    DVASSERT(parentWidget);
    return parentWidget;
}
