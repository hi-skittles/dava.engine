#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>

class QualitySettingsChildCreator : public DAVA::TArc::ChildCreatorExtension
{
public:
    void ExposeChildren(const std::shared_ptr<DAVA::TArc::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::TArc::PropertyNode>>& children) const override;
};

class QualitySettingsEditorCreator : public DAVA::TArc::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::TArc::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::TArc::PropertyNode>& node) const override;
};