#include "Entity/Component.h"
#include "Scene3D/Systems/DebugRenderSystem.h"
#include "Debug/DVAssert.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/DebugRenderComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/CameraComponent.h"
#include "Scene3D/Components/LightComponent.h"

#include "Render/Highlevel/Camera.h"
#include "Render/RenderHelper.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Render/Renderer.h"

namespace DAVA
{
DebugRenderSystem::DebugRenderSystem(Scene* scene)
    : SceneSystem(scene)
    ,
    camera(0)
{
}

DebugRenderSystem::~DebugRenderSystem()
{
}

void DebugRenderSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_DEBUG_RENDER_SYSTEM);

    SetCamera(GetScene()->GetCurrentCamera());

    /*    uint32 size = static_cast<uint32>(entities.size());
	for(uint32 i = 0; i < size; ++i)
	{
        Entity * entity = entities[i];
        
        DebugRenderComponent * debugRenderComponent = entity->GetComponent<DebugRenderComponent>(DebugRenderComponent);
        TransformComponent * transformComponent = entity->GetComponent<TransformComponent>();
        //RenderComponent * renderComponent = entity->GetComponent<RenderComponent>();
        
        //Matrix4 worldTransform = camera->GetMatrix();
        Renderer::GetDynamicBindings().SetDynamicParam(PARAM_VIEW, &camera->GetMatrix(), DynamicBindings::UPDATE_SEMANTIC_ALWAYS);

        AABBox3 debugBoundigBox = entity->GetWTMaximumBoundingBoxSlow();
        uint32 debugFlags = debugRenderComponent->GetDebugFlags();

		// Camera debug draw
		if(debugFlags & DebugRenderComponent::DEBUG_DRAW_CAMERA)
		{
			CameraComponent * entityCameraComp = entity->GetComponent<CameraComponent>();

			if(NULL != entityCameraComp)
			{
				Camera* entityCamera = entityCameraComp->GetCamera();
				if(NULL != entityCamera && camera != entityCamera)
				{
					Color camColor(0.0f, 1.0f, 0.0f, 1.0f);
					Vector3 camPos = entityCamera->GetPosition();
					//Vector3 camDirect = entityCamera->GetDirection();
					AABBox3 camBox(camPos, 2.5f);

					// If this is clip camera - show it as red camera
					if (entityCamera == entity->GetScene()->GetDrawCamera()) camColor = Color(1.0f, 0.0f, 0.0f, 1.0f);


					RenderSystem2D::Instance()->SetColor(camColor);
					RenderHelper::Instance()->DrawBox(camBox, 2.5f, depthWriteState);

					//RenderManager::Instance()->SetState(RenderState::DEFAULT_3D_STATE);
					RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

					debugBoundigBox = camBox;
				}
			}
		}

		// UserNode debug draw
		if(debugFlags & DebugRenderComponent::DEBUG_DRAW_USERNODE)
		{
			if(NULL != entity->GetComponent<UserComponent>())
			{
				Color dcColor(0.0f, 0.0f, 1.0f, 1.0f);
				AABBox3 dcBox(Vector3(), 1.0f);

				//Matrix4 prevMatrix = RenderManager::Instance()->GetMatrix(RenderManager::MATRIX_MODELVIEW);
				//Matrix4 finalMatrix = transformComponent->GetWorldMatrix() * prevMatrix;
				
                Renderer::GetDynamicBindings().SetDynamicParam(PARAM_WORLD, &transformComponent->GetWorldMatrix(), DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
                Renderer::GetDynamicBindings().SetDynamicParam(PARAM_VIEW, &camera->GetMatrix(), DynamicBindings::UPDATE_SEMANTIC_ALWAYS);

								
				RenderSystem2D::Instance()->SetColor(1.f, 1.f, 0, 1.0f);
				RenderHelper::Instance()->DrawLine(Vector3(0, 0, 0), Vector3(1.f, 0, 0), 1.0f, depthTestState);
				RenderSystem2D::Instance()->SetColor(1.f, 0, 1.f, 1.0f);
				RenderHelper::Instance()->DrawLine(Vector3(0, 0, 0), Vector3(0, 1.f, 0), 1.0f, depthTestState);
				RenderSystem2D::Instance()->SetColor(0, 1.f, 1.f, 1.0f);
				RenderHelper::Instance()->DrawLine(Vector3(0, 0, 0), Vector3(0, 0, 1.f), 1.0f, depthTestState);

				RenderSystem2D::Instance()->SetColor(dcColor);
				RenderHelper::Instance()->DrawBox(dcBox, 1.0f, depthTestState);

				//RenderManager::Instance()->SetState(RenderState::DEFAULT_3D_STATE);
				RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
				//RenderManager::Instance()->SetMatrix(RenderManager::MATRIX_MODELVIEW, prevMatrix);

				dcBox.GetTransformedBox(transformComponent->GetWorldMatrix(), debugBoundigBox);
			}
		}

		// LightNode debug draw
		if (debugFlags & DebugRenderComponent::DEBUG_DRAW_LIGHT_NODE)
		{
			LightComponent *lightComp = entity->GetComponent<LightComponent>();

			if(NULL != lightComp)
			{
				Light* light = lightComp->GetLightObject();

				if(NULL != light)
				{
					Vector3 lPosition = light->GetPosition();

					RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 0.0f, 1.0f);

					switch (light->GetType())
					{
					case Light::TYPE_DIRECTIONAL:
						{
							Vector3 lDirection = light->GetDirection();

							lDirection.Normalize();
							RenderHelper::Instance()->DrawArrow(lPosition, lPosition + lDirection * 10, 2.5f, 1.0f, depthWriteState);
							RenderHelper::Instance()->DrawBox(AABBox3(lPosition, 0.5f), 1.5f, depthWriteState);

							debugBoundigBox = AABBox3(lPosition, 2.5f);
						}
						break;
					default:
						{
							AABBox3 lightBox(lPosition, 2.5f);
							RenderHelper::Instance()->DrawBox(lightBox, 2.5f, depthWriteState);

							debugBoundigBox = lightBox;
						}
						break;
					}

					//RenderManager::Instance()->SetState(RenderState::DEFAULT_3D_STATE);
					RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}
		}
        
        if ((debugFlags & DebugRenderComponent::DEBUG_DRAW_AABOX_CORNERS))
        {
            RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
            RenderHelper::Instance()->DrawCornerBox(debugBoundigBox, 1.0f, depthTestState);
        }
        
        if (debugFlags & DebugRenderComponent::DEBUG_DRAW_RED_AABBOX)
        {
            RenderSystem2D::Instance()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
            RenderHelper::Instance()->DrawBox(debugBoundigBox, 1.0f, depthWriteState);
            RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        }               
    }*/
}

void DebugRenderSystem::AddEntity(Entity* entity)
{
    entities.push_back(entity);

    //DebugRenderComponent * debugRenderComponent = entity->GetComponent<DebugRenderComponent>();
    RenderComponent* renderComponent = entity->GetComponent<RenderComponent>();
    if (renderComponent)
    {
        //renderComponent->renderObject->SetDebugFlags(debugRenderComponent->GetFlags());
    }
}

void DebugRenderSystem::RemoveEntity(Entity* entity)
{
    //DebugRenderComponent * debugRenderComponent = entity->GetComponent<DebugRenderComponent>();
    RenderComponent* renderComponent = entity->GetComponent<RenderComponent>();
    if (renderComponent)
    {
        //renderComponent->renderObject->SetDebugFlags(0);
    }

    uint32 size = static_cast<uint32>(entities.size());
    for (uint32 i = 0; i < size; ++i)
    {
        if (entities[i] == entity)
        {
            entities[i] = entities[size - 1];
            entities.pop_back();
            return;
        }
    }

    DVASSERT(0);
}

void DebugRenderSystem::PrepareForRemove()
{
    entities.clear();
}

void DebugRenderSystem::SetCamera(Camera* _camera)
{
    camera = _camera;
}
}
