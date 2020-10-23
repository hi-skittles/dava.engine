#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>

class QualitySettingsChildCreator : public DAVA::ChildCreatorExtension
{
public:
    void ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const override;
};

class QualitySettingsEditorCreator : public DAVA::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const override;
};