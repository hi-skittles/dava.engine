#pragma once

#include "Base/BaseTypes.h"

#include "Model/PackageHierarchy/PackageBaseNode.h"
#include "Model/PackageHierarchy/ControlNode.h"

using SelectedNodes = DAVA::Set<PackageBaseNode*>;
using SelectedControls = DAVA::Set<ControlNode*>;

struct SelectionContainer
{
    void GetNotExistedItems(const SelectedNodes& in, SelectedNodes& out);

    void GetOnlyExistedItems(const SelectedNodes& in, SelectedNodes& out);

    void MergeSelection(const SelectedNodes& selected, const SelectedNodes& deselected);

    template <typename ContainerOut>
    static void MergeSelectionToContainer(const SelectedNodes& selected, const SelectedNodes& deselected, ContainerOut& out);

    SelectedNodes selectedNodes;
};

inline void SelectionContainer::GetNotExistedItems(const SelectedNodes& in, SelectedNodes& out)
{
    std::set_difference(in.begin(), in.end(), selectedNodes.begin(), selectedNodes.end(), std::inserter(out, out.end()));
}

inline void SelectionContainer::GetOnlyExistedItems(const SelectedNodes& in, SelectedNodes& out)
{
    std::set_intersection(selectedNodes.begin(), selectedNodes.end(), in.begin(), in.end(), std::inserter(out, out.end()));
}

inline void SelectionContainer::MergeSelection(const SelectedNodes& selected, const SelectedNodes& deselected)
{
    for (const auto& node : deselected)
    {
        selectedNodes.erase(node);
    }
    for (const auto& node : selected)
    {
        selectedNodes.insert(node);
    }
}

template <typename ContainerOut>
inline void SelectionContainer::MergeSelectionToContainer(const SelectedNodes& selected, const SelectedNodes& deselected, ContainerOut& out)
{
    using T = typename std::remove_reference<ContainerOut>::type::value_type;
    for (const auto& node : deselected)
    {
        T item = dynamic_cast<T>(node);
        out.erase(item);
    }
    for (const auto& node : selected)
    {
        T item = dynamic_cast<T>(node);
        if (nullptr != item)
        {
            out.insert(item);
        }
    }
}

namespace DAVA
{
template <>
struct AnyCompare<SelectedNodes>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        const SelectedNodes& s1 = v1.Get<SelectedNodes>();
        const SelectedNodes& s2 = v2.Get<SelectedNodes>();
        return s1 == s2;
    }
};
}
