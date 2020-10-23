#include "Scene3D/Components/StaticOcclusionComponent.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Render/Highlevel/RenderObject.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(StaticOcclusionDataComponent)
{
    ReflectionRegistrator<StaticOcclusionDataComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .Field("sizeInKBytes", &StaticOcclusionDataComponent::GetDataSize, &StaticOcclusionDataComponent::SetDataSize)[M::ReadOnly(), M::DisplayName("Size in kBytes")]
    .End();
}

DAVA_VIRTUAL_REFLECTION_IMPL(StaticOcclusionComponent)
{
    ReflectionRegistrator<StaticOcclusionComponent>::Begin()
    .ConstructorByPointer()
    .Field("bbox", &StaticOcclusionComponent::GetBoundingBox, &StaticOcclusionComponent::SetBoundingBox)[M::DisplayName("Bounding Box")]
    .Field("subdivX", &StaticOcclusionComponent::GetSubdivisionsX, &StaticOcclusionComponent::SetSubdivisionsX)[M::DisplayName("Subdivisions X"), M::Range(1, Any(), 1)]
    .Field("subdivY", &StaticOcclusionComponent::GetSubdivisionsY, &StaticOcclusionComponent::SetSubdivisionsY)[M::DisplayName("Subdivisions Y"), M::Range(1, Any(), 1)]
    .Field("subdivZ", &StaticOcclusionComponent::GetSubdivisionsZ, &StaticOcclusionComponent::SetSubdivisionsZ)[M::DisplayName("Subdivisions Z"), M::Range(1, Any(), 1)]
    .Field("placeOnLandScape", &StaticOcclusionComponent::GetPlaceOnLandscape, &StaticOcclusionComponent::SetPlaceOnLandscape)[M::DisplayName("Place on Landscape")]
    .Field("occlusionPixelThreshold", &StaticOcclusionComponent::GetOcclusionPixelThreshold, &StaticOcclusionComponent::SetOcclusionPixelThreshold)[M::DisplayName("Occlusion Pixel Threshold")]
    .Field("occlusionPixelThresholdForSpeedtree", &StaticOcclusionComponent::GetOcclusionPixelThresholdForSpeedtree, &StaticOcclusionComponent::SetOcclusionPixelThresholdForSpeedtree)[M::DisplayName("Occlusion Pixel Threshold For Speedtree")]
    .End();
}

DAVA_VIRTUAL_REFLECTION_IMPL(StaticOcclusionDebugDrawComponent)
{
    ReflectionRegistrator<StaticOcclusionDebugDrawComponent>::Begin()
    [M::CantBeCreatedManualyComponent(), M::CantBeDeletedManualyComponent(), M::NonExportableComponent(), M::NonSerializableComponent()]
    .ConstructorByPointer()
    .End();
}

StaticOcclusionComponent::StaticOcclusionComponent()
{
    xSubdivisions = 2;
    ySubdivisions = 2;
    zSubdivisions = 2;
    boundingBox = AABBox3(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f, 20.0f, 20.0f));
    placeOnLandscape = false;
    occlusionPixelThreshold = 0;
    occlusionPixelThresholdForSpeedtree = 0;
}

Component* StaticOcclusionComponent::Clone(Entity* toEntity)
{
    StaticOcclusionComponent* newComponent = new StaticOcclusionComponent();
    newComponent->SetEntity(toEntity);
    newComponent->SetSubdivisionsX(xSubdivisions);
    newComponent->SetSubdivisionsY(ySubdivisions);
    newComponent->SetSubdivisionsZ(zSubdivisions);
    newComponent->SetBoundingBox(boundingBox);
    newComponent->SetPlaceOnLandscape(placeOnLandscape);
    newComponent->cellHeightOffset = cellHeightOffset;
    newComponent->SetOcclusionPixelThreshold(occlusionPixelThreshold);
    newComponent->SetOcclusionPixelThresholdForSpeedtree(occlusionPixelThresholdForSpeedtree);
    return newComponent;
}

void StaticOcclusionComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (NULL != archive)
    {
        archive->SetVariant("soc.aabbox", VariantType(boundingBox));
        archive->SetUInt32("soc.xsub", xSubdivisions);
        archive->SetUInt32("soc.ysub", ySubdivisions);
        archive->SetUInt32("soc.zsub", zSubdivisions);
        archive->SetBool("soc.placeOnLandscape", placeOnLandscape);
        archive->SetUInt32("soc.occlusionPixelThreshold", occlusionPixelThreshold);
        archive->SetUInt32("soc.occlusionPixelThresholdForSpeedtree", occlusionPixelThresholdForSpeedtree);
        if (placeOnLandscape)
            archive->SetByteArray("soc.cellHeightOffset", reinterpret_cast<uint8*>(&cellHeightOffset.front()), xSubdivisions * ySubdivisions * sizeof(float32));
    }
}

void StaticOcclusionComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (NULL != archive)
    {
        boundingBox = archive->GetVariant("soc.aabbox")->AsAABBox3();
        xSubdivisions = archive->GetUInt32("soc.xsub", 1);
        ySubdivisions = archive->GetUInt32("soc.ysub", 1);
        zSubdivisions = archive->GetUInt32("soc.zsub", 1);
        occlusionPixelThreshold = archive->GetUInt32("soc.occlusionPixelThreshold", 0);
        occlusionPixelThresholdForSpeedtree = archive->GetUInt32("soc.occlusionPixelThresholdForSpeedtree", 0);
        placeOnLandscape = archive->GetBool("soc.placeOnLandscape", false);
        if (placeOnLandscape)
        {
            cellHeightOffset.resize(xSubdivisions * ySubdivisions, 0);
            DVASSERT(xSubdivisions * ySubdivisions * sizeof(float32) == static_cast<uint32>(archive->GetByteArraySize("soc.cellHeightOffset")));
            memcpy(&cellHeightOffset.front(), archive->GetByteArray("soc.cellHeightOffset"), xSubdivisions * ySubdivisions * sizeof(float32));
        }
    }

    Component::Deserialize(archive, serializationContext);
}

StaticOcclusionDataComponent::StaticOcclusionDataComponent()
{
}

StaticOcclusionDataComponent::~StaticOcclusionDataComponent()
{
}

Component* StaticOcclusionDataComponent::Clone(Entity* toEntity)
{
    StaticOcclusionDataComponent* newComponent = new StaticOcclusionDataComponent();
    newComponent->SetEntity(toEntity);
    newComponent->data = data;
    return newComponent;
}

void StaticOcclusionDataComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (NULL != archive)
    {
        // VB:
        archive->SetVariant("sodc.bbox", VariantType(data.bbox));
        archive->SetUInt32("sodc.blockCount", data.blockCount);
        archive->SetUInt32("sodc.objectCount", data.objectCount);
        archive->SetUInt32("sodc.subX", data.sizeX);
        archive->SetUInt32("sodc.subY", data.sizeY);
        archive->SetUInt32("sodc.subZ", data.sizeZ);
        archive->SetByteArray("sodc.data", reinterpret_cast<const uint8*>(data.GetData()), data.blockCount * data.objectCount / 32 * sizeof(uint32));
        if (data.cellHeightOffset)
            archive->SetByteArray("sodc.cellHeightOffset", reinterpret_cast<uint8*>(data.cellHeightOffset), data.sizeX * data.sizeY * sizeof(float32));
    }
}

void StaticOcclusionDataComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (NULL != archive)
    {
        data.bbox = archive->GetVariant("sodc.bbox")->AsAABBox3();
        data.blockCount = archive->GetUInt32("sodc.blockCount", 0);
        data.objectCount = archive->GetUInt32("sodc.objectCount", 0);
        data.sizeX = archive->GetUInt32("sodc.subX", 1);
        data.sizeY = archive->GetUInt32("sodc.subY", 1);
        data.sizeZ = archive->GetUInt32("sodc.subZ", 1);

        auto numElements = data.blockCount * data.objectCount / 32;
        uint32 dataSize = static_cast<uint32>(sizeof(uint32) * numElements);
        DVASSERT(dataSize == archive->GetByteArraySize("sodc.data"));
        data.SetData(reinterpret_cast<const uint32*>(archive->GetByteArray("sodc.data")), dataSize);

        if (archive->IsKeyExists("sodc.cellHeightOffset"))
        {
            data.cellHeightOffset = new float32[data.sizeX * data.sizeY];
            DVASSERT(data.sizeX * data.sizeY * sizeof(float32) == static_cast<uint32>(archive->GetByteArraySize("sodc.cellHeightOffset")));
            memcpy(data.cellHeightOffset, archive->GetByteArray("sodc.cellHeightOffset"), data.sizeX * data.sizeY * sizeof(float32));
        }
    }

    Component::Deserialize(archive, serializationContext);
}

StaticOcclusionDebugDrawComponent::StaticOcclusionDebugDrawComponent(RenderObject* object)
{
    renderObject = SafeRetain(object);
}

StaticOcclusionDebugDrawComponent::~StaticOcclusionDebugDrawComponent()
{
    SafeRelease(renderObject);

    rhi::DeleteVertexBuffer(vertices);
    rhi::DeleteIndexBuffer(gridIndices);
    rhi::DeleteIndexBuffer(coverIndices);
}

RenderObject* StaticOcclusionDebugDrawComponent::GetRenderObject() const
{
    return renderObject;
}

Component* StaticOcclusionDebugDrawComponent::Clone(Entity* toEntity)
{
    RenderObject* clonedRO = NULL;
    if (NULL != renderObject)
    {
        clonedRO = renderObject->Clone(clonedRO);
    }
    StaticOcclusionDebugDrawComponent* component = new StaticOcclusionDebugDrawComponent(clonedRO);
    component->SetEntity(toEntity);

    return component;
}

void StaticOcclusionDebugDrawComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DVASSERT(false && "Should not Serialize debug components. Check entity::save");
}

void StaticOcclusionDebugDrawComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DVASSERT(false && "Should not Deserialize debug components. Check entity::save");
}
}