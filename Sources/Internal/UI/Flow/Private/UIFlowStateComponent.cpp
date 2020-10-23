#include "UI/Flow/UIFlowStateComponent.h"
#include "Base/GlobalEnum.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Utils/Utils.h"

ENUM_DECLARE(DAVA::UIFlowStateComponent::StateType)
{
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::STATE_GROUP, "Group");
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::STATE_SINGLE, "Single");
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::STATE_MULTIPLE, "Multiple");
}

ENUM_DECLARE(DAVA::UIFlowStateComponent::HistoryType)
{
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::HISTORY_NONE, "None");
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::HISTORY_MULTIPLE, "Multiple");
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::HISTORY_SINGLE, "Single");
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::HISTORY_SINGLE_SIBLING, "SingleSibling");
    ENUM_ADD_DESCR(DAVA::UIFlowStateComponent::HISTORY_ONLY_TOP, "OnlyTop");
}

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowStateComponent)
{
    ReflectionRegistrator<UIFlowStateComponent>::Begin()[M::DisplayName("Flow State")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowStateComponent* c) { SafeRelease(c); })
    .Field("stateType", &UIFlowStateComponent::GetStateType, &UIFlowStateComponent::SetStateType)[M::EnumT<StateType>(), M::DisplayName("State")]
    .Field("historyType", &UIFlowStateComponent::GetHistoryType, &UIFlowStateComponent::SetHistoryType)[M::EnumT<HistoryType>(), M::DisplayName("History")]
    .Field("services", &UIFlowStateComponent::GetServicesAsString, &UIFlowStateComponent::SetServicesFromString)[M::DisplayName("Services")]
    .Field("activateEvents", &UIFlowStateComponent::GetActivateEventsAsString, &UIFlowStateComponent::SetActivateEventsFromString)[M::DisplayName("Activate Events")]
    .Field("deactivateEvents", &UIFlowStateComponent::GetDeactivateEventsAsString, &UIFlowStateComponent::SetDeactivateEventsFromString)[M::DisplayName("Deactivate Events")]
    .Field("effectOut", &UIFlowStateComponent::GetEffectOut, &UIFlowStateComponent::SetEffectOut)[M::EnumT<UIFlowTransitionEffectConfig::Effect>(), M::DisplayName("Effect Out")]
    .Field("effectIn", &UIFlowStateComponent::GetEffectIn, &UIFlowStateComponent::SetEffectIn)[M::EnumT<UIFlowTransitionEffectConfig::Effect>(), M::DisplayName("Effect In")]
    .Field("effectDuration", &UIFlowStateComponent::GetEffectDuration, &UIFlowStateComponent::SetEffectDuration)[M::DisplayName("Effect Duration")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIFlowStateComponent);

UIFlowStateComponent::UIFlowStateComponent() = default;

UIFlowStateComponent::UIFlowStateComponent(const UIFlowStateComponent& dst) = default;

UIFlowStateComponent::~UIFlowStateComponent() = default;

UIFlowStateComponent* UIFlowStateComponent::Clone() const
{
    return new UIFlowStateComponent(*this);
}

void UIFlowStateComponent::SetStateType(StateType type)
{
    stateType = type;
}

void UIFlowStateComponent::SetHistoryType(HistoryType type)
{
    historyType = type;
}

void UIFlowStateComponent::SetServices(const Vector<ServiceDescriptor>& services_)
{
    services = services_;
}

void UIFlowStateComponent::SetActivateEvents(const Vector<FastName>& events)
{
    activateEvents = events;
}

void UIFlowStateComponent::SetDeactivateEvents(const Vector<FastName>& events)
{
    deactivateEvents = events;
}

void UIFlowStateComponent::SetEffectConfig(const UIFlowTransitionEffectConfig& config)
{
    effectConfig = config;
}

void UIFlowStateComponent::SetEffectIn(UIFlowTransitionEffectConfig::Effect effect)
{
    effectConfig.effectIn = effect;
}

UIFlowTransitionEffectConfig::Effect UIFlowStateComponent::GetEffectIn() const
{
    return effectConfig.effectIn;
}

void UIFlowStateComponent::SetEffectOut(UIFlowTransitionEffectConfig::Effect effect)
{
    effectConfig.effectOut = effect;
}

UIFlowTransitionEffectConfig::Effect UIFlowStateComponent::GetEffectOut() const
{
    return effectConfig.effectOut;
}

void UIFlowStateComponent::SetEffectDuration(float32 duration)
{
    effectConfig.duration = duration;
}

float32 UIFlowStateComponent::GetEffectDuration() const
{
    return effectConfig.duration;
}

String UIFlowStateComponent::GetServicesAsString() const
{
    String out;
    for (const ServiceDescriptor& s : services)
    {
        out += s.name + "," + s.nativeType + ";";
    }
    return out;
}

void UIFlowStateComponent::SetServicesFromString(const String& servicesString)
{
    services.clear();
    Vector<String> servicesInfosVector;
    Split(servicesString, ";", servicesInfosVector);
    for (String& serviceInfoString : servicesInfosVector)
    {
        Vector<String> serviceInfoVector;
        Split(serviceInfoString, ",", serviceInfoVector);
        DVASSERT(serviceInfoVector.size() == 2);
        if (serviceInfoVector.size() == 2)
        {
            services.push_back(ServiceDescriptor{ serviceInfoVector[0], serviceInfoVector[1] });
        }
    }
}

String UIFlowStateComponent::GetActivateEventsAsString() const
{
    String out;
    for (const FastName& s : activateEvents)
    {
        out += String(s.c_str()) + ";";
    }
    return out;
}

void UIFlowStateComponent::SetActivateEventsFromString(const String& eventsString)
{
    activateEvents.clear();
    Vector<String> eventsVector;
    Split(eventsString, ";", eventsVector);
    for (String& eventString : eventsVector)
    {
        activateEvents.push_back(FastName(eventString));
    }
}

String UIFlowStateComponent::GetDeactivateEventsAsString() const
{
    String out;
    for (const FastName& s : deactivateEvents)
    {
        out += String(s.c_str()) + ";";
    }
    return out;
}

void UIFlowStateComponent::SetDeactivateEventsFromString(const String& eventsString)
{
    deactivateEvents.clear();
    Vector<String> eventsVector;
    Split(eventsString, ";", eventsVector);
    for (String& eventString : eventsVector)
    {
        deactivateEvents.push_back(FastName(eventString));
    }
}
}
