#pragma once

#include "Base/BaseTypes.h"
#include "Base/List.h"

namespace DAVA
{
class UIFlowStateComponent;
class UIFlowStateSystem;

/**
    Different usefull methods to work with Flow states hierarchy.
*/
class UIFlowUtils
{
public:
    /** Build list of activated states from state in top element of specified queue
        to all children and add all found states to this queue. */
    static void BuildActivatedQueue(UIFlowStateSystem* system, List<UIFlowStateComponent*>& queue);
    /** Build list of parent states from state in top element of specified queue
        to root state add all found states to this queue. */
    static void BuildParentsQueue(List<UIFlowStateComponent*>& queue);
    /** Remove states from specified queue which has activate children not
        in this queue. */
    static void RemoveSharingParentsFromQueue(UIFlowStateSystem* system, List<UIFlowStateComponent*>& queue);
    /** Remove already activate parental states from specified queue. */
    static void RemoveActiveParentsFromQueue(UIFlowStateSystem* system, List<UIFlowStateComponent*>& queue);
    /** Remove duplicates of states from specified queue. */
    static void RemoveDuplicates(List<UIFlowStateComponent*>& queue);
    /** Check that specified states are siblings. */
    static bool IsSibling(UIFlowStateComponent* first, UIFlowStateComponent* second);
    /** Find near child of specified state which is Single state. */
    static UIFlowStateComponent* FindNearChildSingleState(UIFlowStateComponent* parent);
};
}
