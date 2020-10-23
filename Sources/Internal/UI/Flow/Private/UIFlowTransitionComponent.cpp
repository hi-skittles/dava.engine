#include "UI/Flow/UIFlowTransitionComponent.h"
#include "Base/GlobalEnum.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Utils/StringFormat.h"
#include "Utils/Utils.h"

ENUM_DECLARE(DAVA::UIFlowTransitionComponent::TransitionCommand)
{
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::ACTIVATE_STATE, "Activate");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::DEACTIVATE_STATE, "Deactivate");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::ACTIVATE_STATE_BACKGROUND, "ActivateBackground");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::DEACTIVATE_STATE_BACKGROUND, "DeactivateBackground");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::PRELOAD_STATE, "Preload");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::PRELOAD_STATE_BACKGROUND, "PreloadBackground");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::SEND_EVENT, "SendEvent");
    ENUM_ADD_DESCR(DAVA::UIFlowTransitionComponent::HISTORY_BACK, "HistoryBack");
};

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowTransitionComponent)
{
    ReflectionRegistrator<UIFlowTransitionComponent>::Begin()[M::DisplayName("Flow Transition")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIFlowTransitionComponent* c) { SafeRelease(c); })
    .Field("transitions", &UIFlowTransitionComponent::GetTransitionMapAsString, &UIFlowTransitionComponent::SetTransitionMapFromString)[M::DisplayName("Transitions")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIFlowTransitionComponent);

UIFlowTransitionComponent::UIFlowTransitionComponent() = default;

UIFlowTransitionComponent::UIFlowTransitionComponent(const UIFlowTransitionComponent& dst) = default;

UIFlowTransitionComponent::~UIFlowTransitionComponent() = default;

UIFlowTransitionComponent* UIFlowTransitionComponent::Clone() const
{
    return new UIFlowTransitionComponent(*this);
}

void UIFlowTransitionComponent::SetTransitionMap(const TransitionMap& transitions)
{
    transitionMap = transitions;
}

void UIFlowTransitionComponent::SetTransitionMapFromString(const String& transitions)
{
    transitionMap.clear();
    Vector<String> pairs;
    Split(transitions, ";", pairs);
    for (String& pair : pairs)
    {
        Vector<String> values;
        Split(pair, ",", values);
        if (values.size() >= 3)
        {
            TransitionRule rule;
            rule.event = FastName(values[0]);
            GlobalEnumMap<TransitionCommand>::Instance()->ToValue(values[1].c_str(), reinterpret_cast<int32&>(rule.operation));
            switch (rule.operation)
            {
            case ACTIVATE_STATE:
            case ACTIVATE_STATE_BACKGROUND:
            case DEACTIVATE_STATE:
            case DEACTIVATE_STATE_BACKGROUND:
            case PRELOAD_STATE:
            case PRELOAD_STATE_BACKGROUND:
                rule.statePath = values[2];
                break;
            case SEND_EVENT:
                rule.sendEvent = FastName(values[2]);
                break;
            case HISTORY_BACK:
                break;
            default:
                DVASSERT(false);
                break;
            };

            transitionMap.push_back(rule);
        }
    }
}

String UIFlowTransitionComponent::GetTransitionMapAsString() const
{
    String out;
    for (const TransitionRule& rule : transitionMap)
    {
        out += String(rule.event.c_str()) + ",";
        out += String(GlobalEnumMap<TransitionCommand>::Instance()->ToString(rule.operation)) + ",";
        switch (rule.operation)
        {
        case ACTIVATE_STATE:
        case ACTIVATE_STATE_BACKGROUND:
        case DEACTIVATE_STATE:
        case DEACTIVATE_STATE_BACKGROUND:
        case PRELOAD_STATE:
        case PRELOAD_STATE_BACKGROUND:
            out += rule.statePath;
            break;
        case SEND_EVENT:
            out += rule.sendEvent.c_str();
            break;
        case HISTORY_BACK:
            out += "@";
            break;
        default:
            DVASSERT(false);
            break;
        }
        out += ";";
    }
    return out;
}
}
