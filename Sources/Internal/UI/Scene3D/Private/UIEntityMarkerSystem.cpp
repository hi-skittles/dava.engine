#include "UI/Scene3D/UIEntityMarkerSystem.h"
#include "Debug/DVAssert.h"
#include "Render/Highlevel/Camera.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/TransformComponent.h"
#include "UI/Scene3D/UIEntityMarkerComponent.h"
#include "UI/Scene3D/UIEntityMarkersContainerComponent.h"
#include "UI/UIControl.h"

#include <Math/Transform.h>

namespace DAVA
{
UIEntityMarkerSystem::UIEntityMarkerSystem() = default;

UIEntityMarkerSystem::~UIEntityMarkerSystem() = default;

void UIEntityMarkerSystem::RegisterControl(UIControl* control)
{
    UIEntityMarkersContainerComponent* container = control->GetComponent<UIEntityMarkersContainerComponent>();
    if (container)
    {
        Link l;
        l.container = container;
        links.push_back(l);

        // Children will be added automatically by RegisterControl
    }

    UIEntityMarkerComponent* marker = control->GetComponent<UIEntityMarkerComponent>();
    if (marker && control->GetParent())
    {
        UIEntityMarkersContainerComponent* container = control->GetParent()->GetComponent<UIEntityMarkersContainerComponent>();
        if (container)
        {
            auto it = std::find_if(links.begin(), links.end(), [container](const Link& l) {
                return l.container == container;
            });
            if (it != links.end())
            {
                it->children.push_back(marker);
            }
        }
    }
}

void UIEntityMarkerSystem::UnregisterControl(UIControl* control)
{
    UIEntityMarkersContainerComponent* container = control->GetComponent<UIEntityMarkersContainerComponent>();
    if (container)
    {
        auto it = std::find_if(links.begin(), links.end(), [container](const Link& l) {
            return l.container == container;
        });
        links.erase(it, links.end());
    }

    UIEntityMarkerComponent* marker = control->GetComponent<UIEntityMarkerComponent>();
    if (marker && control->GetParent())
    {
        UIEntityMarkersContainerComponent* container = control->GetParent()->GetComponent<UIEntityMarkersContainerComponent>();
        if (container)
        {
            auto it = std::find_if(links.begin(), links.end(), [container](const Link& l) {
                return l.container == container;
            });
            if (it != links.end())
            {
                it->children.erase(std::find(it->children.begin(), it->children.end(), marker), it->children.end());
                it->orderMap.erase(marker->GetControl());
            }
        }
    }
}

void UIEntityMarkerSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIEntityMarkersContainerComponent>())
    {
        Link l;
        l.container = static_cast<UIEntityMarkersContainerComponent*>(component);
        links.push_back(l);

        // Check children and add their markers manually
        for (const auto& child : control->GetChildren())
        {
            UIEntityMarkerComponent* marker = child->GetComponent<UIEntityMarkerComponent>();
            if (marker)
            {
                l.children.push_back(marker);
            }
        }
    }

    if (component->GetType() == Type::Instance<UIEntityMarkerComponent>() && control->GetParent())
    {
        UIEntityMarkersContainerComponent* container = control->GetParent()->GetComponent<UIEntityMarkersContainerComponent>();
        if (container)
        {
            auto it = std::find_if(links.begin(), links.end(), [container](const Link& l) {
                return l.container == container;
            });
            if (it != links.end())
            {
                it->children.push_back(static_cast<UIEntityMarkerComponent*>(component));
            }
        }
    }
}

void UIEntityMarkerSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIEntityMarkersContainerComponent>())
    {
        auto it = std::find_if(links.begin(), links.end(), [component](const Link& l) {
            return l.container == static_cast<UIEntityMarkersContainerComponent*>(component);
        });
        links.erase(it, links.end());
    }

    if (component->GetType() == Type::Instance<UIEntityMarkerComponent>() && control->GetParent())
    {
        UIEntityMarkersContainerComponent* container = control->GetParent()->GetComponent<UIEntityMarkersContainerComponent>();
        if (container)
        {
            auto it = std::find_if(links.begin(), links.end(), [container](const Link& l) {
                return l.container == container;
            });
            if (it != links.end())
            {
                UIEntityMarkerComponent* marker = static_cast<UIEntityMarkerComponent*>(component);
                it->children.erase(std::find(it->children.begin(), it->children.end(), marker), it->children.end());
                it->orderMap.erase(marker->GetControl());
            }
        }
    }
}

void UIEntityMarkerSystem::Process(float32 elapsedTime)
{
    for (Link& l : links)
    {
        UnorderedMap<UIControl*, float32>& orderMap = l.orderMap;
        UIEntityMarkersContainerComponent* container = l.container;
        UIControl* containerControl = container->GetControl();

        if (!container->IsEnabled())
        {
            continue;
        }

        for (UIEntityMarkerComponent* marker : l.children)
        {
            UIControl* markerControl = marker->GetControl();

            Entity* target = marker->GetTargetEntity();
            if (target == nullptr || target->GetScene() == nullptr)
            {
                continue;
            }

            Camera* camera = target->GetScene()->GetCurrentCamera();
            if (camera == nullptr)
            {
                continue;
            }

            TransformComponent* targetTransform = target->GetComponent<TransformComponent>();
            Vector3 worldPosition = targetTransform->GetWorldTransform().GetTranslation();
            Vector3 positionAndDepth = camera->GetOnScreenPositionAndDepth(worldPosition, containerControl->GetAbsoluteRect());
            Vector3 distance = worldPosition - camera->GetPosition();

            if (container->IsSyncVisibilityEnabled())
            {
                const bool visible = distance.DotProduct(camera->GetDirection()) > 0.f;
                markerControl->SetVisibilityFlag(visible);

                if (!visible)
                {
                    // Skip next steps for invisible controls
                    continue;
                }
            }

            if (container->IsSyncPositionEnabled())
            {
                markerControl->SetPosition(Vector2(positionAndDepth.x, positionAndDepth.y));
            }

            if (container->IsSyncScaleEnabled())
            {
                if (distance.SquareLength() > 0.f)
                {
                    const Vector2& factor = container->GetScaleFactor();
                    const Vector2& maxScale = container->GetMaxScale();
                    const Vector2& minScale = container->GetMinScale();
                    const float32 dis = distance.Length();

                    Vector2 scale(Clamp(factor.x / dis, minScale.x, maxScale.x),
                                  Clamp(factor.y / dis, minScale.y, maxScale.y));

                    markerControl->SetScale(scale);
                }
                else
                {
                    markerControl->SetScale(container->GetMaxScale());
                }
            }

            if (container->IsSyncOrderEnabled())
            {
                switch (container->GetOrderMode())
                {
                case UIEntityMarkersContainerComponent::OrderMode::NearFront:
                    orderMap[markerControl] = -distance.SquareLength();
                    break;
                case UIEntityMarkersContainerComponent::OrderMode::NearBack:
                    orderMap[markerControl] = distance.SquareLength();
                    break;
                }
            }

            if (container->IsUseCustomStrategy() && container->GetCustomStrategy())
            {
                container->GetCustomStrategy()(markerControl, container, marker);
            }
        }

        if (container->IsSyncOrderEnabled() && !orderMap.empty())
        {
            containerControl->SortChildren([&orderMap](const RefPtr<UIControl>& a, const RefPtr<UIControl>& b) {
                auto itA = orderMap.find(a.Get());
                auto itB = orderMap.find(b.Get());
                float32 valA = itA != orderMap.end() ? itA->second : 0.f;
                float32 valB = itB != orderMap.end() ? itB->second : 0.f;
                return valA < valB;
            });
        }
    }
}
}
