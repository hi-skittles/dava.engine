#include "Scene3D/Components/RenderComponent.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Base/ObjectFactory.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(RenderComponent)
{
    ReflectionRegistrator<RenderComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .Field("renderObject", &RenderComponent::renderObject)[M::DisplayName("Render Object")]
    .End();
}

RenderComponent::RenderComponent(RenderObject* _object)
{
    renderObject = SafeRetain(_object);
}

RenderComponent::~RenderComponent()
{
    SafeRelease(renderObject);
}

void RenderComponent::SetRenderObject(RenderObject* _renderObject)
{
    SafeRelease(renderObject);
    renderObject = SafeRetain(_renderObject);
}

RenderObject* RenderComponent::GetRenderObject()
{
    return renderObject;
}

Component* RenderComponent::Clone(Entity* toEntity)
{
    RenderComponent* component = new RenderComponent();
    component->SetEntity(toEntity);

    if (NULL != renderObject)
    {
        //TODO: Do not forget ot check what does it means.
        component->renderObject = renderObject->Clone(component->renderObject);
    }

    return component;
}

void RenderComponent::OptimizeBeforeExport()
{
    /*
    uint32 count = renderObject->GetRenderBatchCount();
    for(uint32 i = 0; i < count; ++i)
    {
        RenderBatch *renderBatch = renderObject->GetRenderBatch(i);
		if(NULL != renderBatch)
		{
			PolygonGroup* polygonGroup = renderBatch->GetPolygonGroup();
			NMaterial* material = renderBatch->GetMaterial();
			if(NULL != polygonGroup && NULL != material)
			{
				//uint32 newFormat = MaterialOptimizer::GetOptimizedVertexFormat((Material::eType)renderBatch->GetMaterial()->type);
				// polygonGroup->OptimizeVertices(newFormat); //TODO::VK crash on Tanks/USSR/T-28_crash.sc2
			}
		}
	}
*/
}

void RenderComponent::GetDataNodes(Set<DAVA::DataNode*>& dataNodes)
{
    if (NULL != renderObject)
    {
        renderObject->GetDataNodes(dataNodes);
    }
}

void RenderComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (NULL != archive && NULL != renderObject)
    {
        KeyedArchive* roArch = new KeyedArchive();
        renderObject->Save(roArch, serializationContext);
        archive->SetArchive("rc.renderObj", roArch);
        roArch->Release();
    }
}

void RenderComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (NULL != archive)
    {
        KeyedArchive* roArch = archive->GetArchive("rc.renderObj");
        if (NULL != roArch)
        {
            RenderObject* ro = static_cast<RenderObject*>(ObjectFactory::Instance()->New<RenderObject>(roArch->GetString("##name")));
            if (NULL != ro)
            {
                ro->Load(roArch, serializationContext);
                SetRenderObject(ro);

                ro->Release();
            }
        }
    }

    Component::Deserialize(archive, serializationContext);
}
};
