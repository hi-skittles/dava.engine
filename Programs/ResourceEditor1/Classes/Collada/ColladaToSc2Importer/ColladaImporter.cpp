#include "Animation/AnimationClip.h"
#include "Animation/AnimationTrack.h"
#include "Animation/AnimationChannel.h"
#include "FileSystem/DynamicMemoryFile.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/SceneUtils.h"
#include "Scene3D/AnimationData.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/AnimationComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Render/Highlevel/Mesh.h"
#include "Render/3D/MeshUtils.h"
#include "Render/3D/PolygonGroup.h"
#include "Collada/ColladaMeshInstance.h"
#include "Collada/ColladaSceneNode.h"
#include "Collada/ColladaScene.h"
#include "Collada/ColladaToSc2Importer/ColladaImporter.h"
#include "Collada/ColladaToSc2Importer/ImportSettings.h"
#include "Utils/UTF8Utils.h"
#include "Utils/CRC32.h"
#include "Qt/Main/QtUtils.h"

namespace DAVA
{
ColladaImporter::ColladaImporter()
{
}

// Creates Dava::RenderObject from ColladaMeshInstance and puts it
RenderObject* ColladaImporter::GetMeshFromCollada(ColladaMeshInstance* mesh, const FastName& name)
{
    ColladaSkinnedMesh* colladaSkinnedMesh = mesh->GetSkinnedMesh();
    bool isSkinnedMesh = colladaSkinnedMesh != nullptr;

    RenderObject* ro = nullptr;
    if (isSkinnedMesh)
    {
        ro = new SkinnedMesh();
    }
    else
    {
        ro = new Mesh();
    }

    for (auto polygonGroupInstance : mesh->polyGroupInstances)
    {
        PolygonGroup* davaPolygon = library.GetOrCreatePolygon(polygonGroupInstance);

        bool isShadow = (strstr(name.c_str(), ImportSettings::shadowNamePattern.c_str()) != nullptr);
        if (isShadow)
        {
            davaPolygon = DAVA::MeshUtils::CreateShadowPolygonGroup(davaPolygon);
        }

        ScopedPtr<NMaterial> davaMaterial(library.CreateMaterialInstance(polygonGroupInstance, isShadow));
        if (isSkinnedMesh)
        {
            SkinnedMesh* davaMesh = static_cast<SkinnedMesh*>(ro);

            DVASSERT(polygonGroupInstance->polyGroup->maxVertexInfluenceCount > 0);
            uint32 maxJointWeights = polygonGroupInstance->polyGroup->maxVertexInfluenceCount;

            if (davaPolygon->GetFormat() & EVF_HARD_JOINTINDEX)
                davaMaterial->AddFlag(NMaterialFlagName::FLAG_HARD_SKINNING, 1);
            else
                davaMaterial->AddFlag(NMaterialFlagName::FLAG_SOFT_SKINNING, maxJointWeights);

            auto skinnedDavaPolygons = MeshUtils::SplitSkinnedMeshGeometry(davaPolygon, SkinnedMesh::MAX_TARGET_JOINTS);
            for (auto& p : skinnedDavaPolygons)
            {
                PolygonGroup* pg = p.first;
                pg->ApplyMatrix(colladaSkinnedMesh->bindShapeMatrix);
                pg->RecalcAABBox();

                ScopedPtr<RenderBatch> davaBatch(new RenderBatch());
                davaBatch->SetPolygonGroup(pg);
                davaBatch->SetMaterial(davaMaterial);

                davaMesh->AddRenderBatch(davaBatch);
                davaMesh->SetJointTargets(davaBatch, p.second);

                SafeRelease(pg);
            }
        }
        else
        {
            ScopedPtr<RenderBatch> davaBatch(new RenderBatch());

            davaBatch->SetPolygonGroup(davaPolygon);
            davaBatch->SetMaterial(davaMaterial);
            ro->AddRenderBatch(davaBatch);
        }
    }
    // TO VERIFY?
    DVASSERT(0 < ro->GetRenderBatchCount() && "Empty mesh");
    return ro;
}

bool ColladaImporter::VerifyColladaMesh(ColladaMeshInstance* mesh, const FastName& nodeName)
{
    for (auto polygonGroupInstance : mesh->polyGroupInstances)
    {
        if (polygonGroupInstance->material == nullptr)
        {
            Logger::Error("[DAE to SC2] Node %s has no material", nodeName.c_str());
            return false;
        }

        auto polyGroup = polygonGroupInstance->polyGroup;
        if ((polyGroup == nullptr) || polyGroup->GetVertices().empty())
        {
            Logger::Error("[DAE to SC2] Node %s has no geometric data", nodeName.c_str());
            return false;
        }
    }

    return true;
}

eColladaErrorCodes ColladaImporter::VerifyDavaMesh(RenderObject* mesh, const FastName name)
{
    eColladaErrorCodes retValue = eColladaErrorCodes::COLLADA_OK;

    uint32 batchesCount = mesh->GetRenderBatchCount();
    if (0 == batchesCount)
    {
        Logger::Error("[DAE to SC2] %s has no render batches.", name.c_str());
        retValue = eColladaErrorCodes::COLLADA_ERROR;
    }
    else
    {
        for (uint32 i = 0; i < batchesCount; ++i)
        {
            auto batch = mesh->GetRenderBatch(i);
            if (nullptr == batch)
            {
                Logger::Error("[DAE to SC2] Node %s has no %i render batch", i);
                retValue = eColladaErrorCodes::COLLADA_ERROR;
            }

            auto polygon = batch->GetPolygonGroup();
            if (nullptr == polygon)
            {
                Logger::Error("[DAE to SC2] Node %s has no polygon in render batch %i ", i);
                retValue = eColladaErrorCodes::COLLADA_ERROR;
            }

            if (0 >= polygon->GetVertexCount())
            {
                Logger::Error("[DAE to SC2] Node %s has no geometric data", name.c_str());
                retValue = eColladaErrorCodes::COLLADA_ERROR;
            }
        }
    }

    return retValue;
}

eColladaErrorCodes ColladaImporter::ImportMeshes(const Vector<ColladaMeshInstance*>& meshInstances, Entity* node)
{
    eColladaErrorCodes retValue = eColladaErrorCodes::COLLADA_OK;

    DVASSERT(meshInstances.size() <= 1 && "Should be only one meshInstance in one collada node");
    for (auto meshInstance : meshInstances)
    {
        if (VerifyColladaMesh(meshInstance, node->GetName()))
        {
            ScopedPtr<RenderObject> renderObject(GetMeshFromCollada(meshInstance, node->GetName()));
            RenderComponent* davaRenderComponent = GetRenderComponent(node);
            if (nullptr == davaRenderComponent)
            {
                davaRenderComponent = new RenderComponent();
                node->AddComponent(davaRenderComponent);
            }
            davaRenderComponent->SetRenderObject(renderObject);

            // Verification!
            eColladaErrorCodes iterationRet = VerifyDavaMesh(renderObject.get(), node->GetName());
            retValue = Max(iterationRet, retValue);
        }
        else
        {
            retValue = eColladaErrorCodes::COLLADA_ERROR;
        }
    }

    return retValue;
}

void ColladaImporter::ImportAnimation(ColladaSceneNode* colladaNode, Entity* nodeEntity)
{
    if (nullptr != colladaNode->animation)
    {
        auto* animationComponent = new AnimationComponent();
        animationComponent->SetEntity(nodeEntity);
        nodeEntity->AddComponent(animationComponent);

        // Calculate actual transform and bake it into animation keys.
        // NOTE: for now usage of the same animation more than once is bad idea
        // NOTE: animation->BakeTransform(totalTransform); actually was removed because of poor rotation behavior
        // when pose baked into keys. So just set correct invPose to animation instead.
        AnimationData* animation = library.GetOrCreateAnimation(colladaNode->animation);
        Matrix4 totalTransform = colladaNode->AccumulateTransformUptoFarParent(colladaNode->scene->rootNode);

        Matrix4 invPose;
        totalTransform.GetInverse(invPose);
        animation->SetInvPose(invPose);
        animationComponent->SetAnimation(animation);
    }
}

void ColladaImporter::ImportSkeleton(ColladaSceneNode* colladaNode, Entity* node)
{
    DVASSERT(colladaNode->meshInstances.size() <= 1);

    ColladaSkinnedMesh* skinnedMesh = colladaNode->meshInstances.empty() ? nullptr : colladaNode->meshInstances[0]->GetSkinnedMesh();
    if (skinnedMesh == nullptr)
        return;

    SkeletonComponent* davaSkeletonComponent = new SkeletonComponent();
    node->AddComponent(davaSkeletonComponent);

    int32 jointsCount = int32(skinnedMesh->joints.size());
    Vector<SkeletonComponent::Joint> joints(jointsCount);
    for (int32 i = 0; i < jointsCount; ++i)
    {
        const ColladaSkinnedMesh::Joint& colladaJoint = skinnedMesh->joints[i];
        const ColladaSceneNode* jointNode = colladaJoint.node;
        SkeletonComponent::Joint& joint = joints[i];

        DVASSERT(colladaJoint.parentIndex < colladaJoint.index);

        bool isRootJoint = (colladaJoint.parentIndex == -1);

        joint.parentIndex = isRootJoint ? SkeletonComponent::INVALID_JOINT_INDEX : colladaJoint.parentIndex;
        joint.name = FastName(colladaJoint.jointName);
        joint.uid = FastName(colladaJoint.jointUID);

        joint.bindTransform = isRootJoint ? jointNode->AccumulateTransformUptoFarParent(colladaNode->scene->rootNode) : jointNode->localTransform;
        joint.bindTransformInv = colladaJoint.inverse0;

        joint.bbox = AABBox3(Vector3(), Vector3());
    }

    //TODO: *Skinning* calc bounding sphere instead bbox?
    const Matrix4& bindShapeMatrix = skinnedMesh->bindShapeMatrix;
    for (auto& pgi : colladaNode->meshInstances[0]->polyGroupInstances)
    {
        PolygonGroup* polygonGroup = library.GetOrCreatePolygon(pgi);
        int32 jointIndex = -1;
        float32 jointWeight = 0.f;
        Vector3 position;
        int32 vxCount = polygonGroup->GetVertexCount();
        int32 vertexFormat = polygonGroup->GetFormat();
        for (int32 v = 0; v < vxCount; ++v)
        {
            if (vertexFormat & (EVF_JOINTINDEX | EVF_JOINTWEIGHT)) //soft-skinning
            {
                for (int32 j = 0; j < 4; ++j)
                {
                    polygonGroup->GetJointWeight(v, j, jointWeight);
                    if (jointWeight > EPSILON)
                    {
                        polygonGroup->GetCoord(v, position);
                        polygonGroup->GetJointIndex(v, j, jointIndex);

                        joints[jointIndex].bbox.AddPoint(position * bindShapeMatrix * joints[jointIndex].bindTransformInv);
                    }
                }
            }
            else
            {
                DVASSERT(vertexFormat & EVF_HARD_JOINTINDEX);

                polygonGroup->GetCoord(v, position);
                polygonGroup->GetHardJointIndex(v, jointIndex);
                joints[jointIndex].bbox.AddPoint(position * bindShapeMatrix * joints[jointIndex].bindTransformInv);
            }
        }
    }

    davaSkeletonComponent->SetJoints(joints);
}

eColladaErrorCodes ColladaImporter::BuildSceneAsCollada(Entity* root, ColladaSceneNode* colladaNode)
{
    eColladaErrorCodes res = eColladaErrorCodes::COLLADA_OK;

    if (colladaNode->originalNode->GetJointFlag())
        return res;

    ScopedPtr<Entity> nodeEntity(new Entity());
    String name = UTF8Utils::MakeUTF8String(colladaNode->originalNode->GetName().c_str());
    if (name.empty())
    {
        name = Format("UNNAMED");

        res = eColladaErrorCodes::COLLADA_ERROR;
        Logger::Error("[DAE to SC2] Unnamed node found as a child of %s", root->GetName().c_str());
        if (0 < colladaNode->childs.size())
        {
            Logger::Error("[DAE to SC2] It's childs:");
            for (auto child : colladaNode->childs)
            {
                Logger::Error("[DAE to SC2] %s", child->originalNode->GetName().c_str());
            }
        }
    }

    nodeEntity->SetName(FastName(name));

    // take mesh from node and put it into entity's render component
    const eColladaErrorCodes importMeshesResult = ImportMeshes(colladaNode->meshInstances, nodeEntity);
    res = Max(importMeshesResult, res);

    // Import skeleton
    ImportSkeleton(colladaNode, nodeEntity);

    // Import animation
    ImportAnimation(colladaNode, nodeEntity);

    auto* transformComponent = GetTransformComponent(nodeEntity);
    transformComponent->SetLocalTransform(&colladaNode->localTransform);

    root->AddNode(nodeEntity);

    for (auto childNode : colladaNode->childs)
    {
        const eColladaErrorCodes childRes = BuildSceneAsCollada(nodeEntity, childNode);
        res = Max(res, childRes);
    }

    return res;
}

void ColladaImporter::LoadMaterialParents(ColladaScene* colladaScene)
{
    for (auto cmaterial : colladaScene->colladaMaterials)
    {
        NMaterial* globalMaterial = library.GetOrCreateMaterialParent(cmaterial, false);
        DVASSERT(nullptr != globalMaterial);
    }
}

void ColladaImporter::LoadAnimations(ColladaScene* colladaScene)
{
    for (auto canimation : colladaScene->colladaAnimations)
    {
        for (auto& pair : canimation->animations)
        {
            SceneNodeAnimation* colladaAnimation = pair.second;
            AnimationData* animation = library.GetOrCreateAnimation(colladaAnimation);
            DVASSERT(nullptr != animation);
        }
    }
}

eColladaErrorCodes ColladaImporter::SaveSC2(ColladaScene* colladaScene, const FilePath& scenePath)
{
    ScopedPtr<Scene> scene(new Scene());

    // Load scene global materials.
    LoadMaterialParents(colladaScene);

    // Load scene global animations
    LoadAnimations(colladaScene);

    // Iterate recursive over collada scene and build Dava Scene with same hierarchy

    eColladaErrorCodes convertRes = BuildSceneAsCollada(scene, colladaScene->rootNode);
    if (eColladaErrorCodes::COLLADA_OK == convertRes)
    {
        // Apply transforms to render batches and use identity local transforms
        SceneUtils::BakeTransformsUpToFarParent(scene, scene);

        // post process Entities and create Lod nodes.
        bool combinedSuccessfull = SceneUtils::CombineLods(scene);
        if (combinedSuccessfull)
        {
            SceneFileV2::eError saveRes = scene->SaveScene(scenePath);
            if (saveRes > SceneFileV2::eError::ERROR_NO_ERROR)
            {
                Logger::Error("[DAE to SC2] Cannot save SC2. Error %d", saveRes);
                convertRes = eColladaErrorCodes::COLLADA_ERROR;
            }
        }
        else
        {
            convertRes = eColladaErrorCodes::COLLADA_ERROR;
        }
    }

    return convertRes;
}

void WriteToBuffer(Vector<uint8>& buffer, const void* data, uint32 size)
{
    DVASSERT(data != nullptr && size != 0);

    const uint8* bytes = reinterpret_cast<const uint8*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

template <class T>
void WriteToBuffer(Vector<uint8>& buffer, const T* value)
{
    WriteToBuffer(buffer, value, sizeof(T));
}

void WriteToBuffer(Vector<uint8>& buffer, const String& string)
{
    uint32 stringBytes = uint32(string.length() + 1);
    WriteToBuffer(buffer, string.c_str(), stringBytes);

    //as we load animation data directly to memory and use it without any processing we have to align strings data
    uint32 stringAlignment = 4 - (stringBytes & 0x3);
    if (stringAlignment > 0 && stringAlignment < 4)
    {
        uint32 pad = 0;
        WriteToBuffer(buffer, &pad, stringAlignment);
    }
}

eColladaErrorCodes ColladaImporter::SaveAnimations(ColladaScene* colladaScene, const FilePath& dir)
{
    //binary file format described in 'AnimationBinaryFormat.md'
    struct ChannelHeader
    {
        //Track part
        uint8 target;
        uint8 pad0[3];

        //Channel part
        uint32 signature = AnimationChannel::ANIMATION_CHANNEL_DATA_SIGNATURE;
        uint8 dimension = 0;
        uint8 interpolation = 0;
        uint16 compression = 0;
        uint32 keyCount = 0;
    } channelHeader;

    for (auto canimation : colladaScene->colladaAnimations)
    {
        FilePath filePath = dir + String(canimation->name + ".anim");
        ScopedPtr<File> file(File::Create(filePath, File::CREATE | File::WRITE));
        if (file)
        {
            Vector<uint8> animationClipData;

            WriteToBuffer(animationClipData, &canimation->duration);

            uint32 nodeCount = uint32(canimation->animationsData.size());
            WriteToBuffer(animationClipData, &nodeCount);

            //Write nodes data
            for (auto& pair : canimation->animationsData)
            {
                ColladaSceneNode* colladaNode = pair.first;
                const ColladaAnimation::ColladaAnimatinData& animationData = pair.second;

                String nodeUID = String(colladaNode->originalNode->GetDaeId().c_str());
                String nodeName = UTF8Utils::EncodeToUTF8(colladaNode->originalNode->GetName().c_str());

                WriteToBuffer(animationClipData, nodeUID);
                WriteToBuffer(animationClipData, nodeName);

                //Write Track data
                uint32 signature = AnimationTrack::ANIMATION_TRACK_DATA_SIGNATURE;
                WriteToBuffer(animationClipData, &signature);

                uint32 channelsCount = 0;
                channelsCount += animationData.translations.empty() ? 0 : 1;
                channelsCount += animationData.rotations.empty() ? 0 : 1;
                channelsCount += animationData.scales.empty() ? 0 : 1;

                WriteToBuffer(animationClipData, &channelsCount);

                if (!animationData.translations.empty())
                {
                    //Write position channel
                    channelHeader.dimension = 3;
                    channelHeader.interpolation = uint8(AnimationChannel::INTERPOLATION_LINEAR);
                    channelHeader.target = AnimationTrack::CHANNEL_TARGET_POSITION;
                    channelHeader.keyCount = uint32(animationData.translations.size());
                    WriteToBuffer(animationClipData, &channelHeader);

                    for (auto& t : animationData.translations)
                    {
                        WriteToBuffer(animationClipData, &t.first); //time
                        WriteToBuffer(animationClipData, &t.second); //position
                    }
                }

                //Write orientation channel
                if (!animationData.rotations.empty())
                {
                    channelHeader.dimension = 4;
                    channelHeader.interpolation = uint8(AnimationChannel::INTERPOLATION_SPHERICAL_LINEAR);
                    channelHeader.target = AnimationTrack::CHANNEL_TARGET_ORIENTATION;
                    channelHeader.keyCount = uint32(animationData.rotations.size());
                    WriteToBuffer(animationClipData, &channelHeader);

                    for (auto& r : animationData.rotations)
                    {
                        WriteToBuffer(animationClipData, &r.first); //time
                        WriteToBuffer(animationClipData, &r.second); //rotation
                    }
                }

                //Write scale channel
                if (!animationData.scales.empty())
                {
                    channelHeader.dimension = 1;
                    channelHeader.interpolation = uint8(AnimationChannel::INTERPOLATION_LINEAR);
                    channelHeader.target = AnimationTrack::CHANNEL_TARGET_SCALE;
                    channelHeader.keyCount = uint32(animationData.scales.size());
                    WriteToBuffer(animationClipData, &channelHeader);

                    for (auto& s : animationData.scales)
                    {
                        WriteToBuffer(animationClipData, &s.first); //time
                        WriteToBuffer(animationClipData, &s.second.x); //scale
                    }
                }
            }

            uint32 markersCount = 0;
            WriteToBuffer(animationClipData, &markersCount);

            uint32 animationDataSize = uint32(animationClipData.size());

            AnimationClip::FileHeader header;
            header.signature = AnimationClip::ANIMATION_CLIP_FILE_SIGNATURE;
            header.version = 1;
            header.crc32 = CRC32::ForBuffer(animationClipData.data(), animationDataSize);
            header.dataSize = animationDataSize;

            file->Write(&header);
            file->Write(animationClipData.data(), animationDataSize);

            file->Flush();
        }
        else
        {
            Logger::Error("[ColladaImporter::SaveAnimations] Failed to open file for writing: %s", filePath.GetAbsolutePathname().c_str());
        }
    }

    return COLLADA_OK;
}
};
