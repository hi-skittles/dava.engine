#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Controls/PropertyPanel/PropertyPanelMeta.h>

namespace PropertyPanel
{
class CollisionTypeChildCreator final : public DAVA::ChildCreatorExtension
{
public:
    void ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const override;
};

class CollisionTypeEditorCreator final : public DAVA::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const override;
};
}
