#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Controls/PropertyPanel/PropertyPanelMeta.h>

namespace PropertyPanel
{
class SlotComponentChildCreator : public DAVA::ChildCreatorExtension
{
public:
    void ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const override;
};

class SlotComponentEditorCreator : public DAVA::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const override;
};

std::shared_ptr<DAVA::M::CommandProducer> CreateCopySlotProducer();
DAVA::M::CommandProducerHolder CreateSlotNameCommandProvider();
DAVA::M::CommandProducerHolder CreateSlotConfigCommandProvider();
}
