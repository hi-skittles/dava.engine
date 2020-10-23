#pragma once

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>

#include <Reflection/ReflectedMeta.h>

class KeyedArchiveChildCreator : public DAVA::TArc::ChildCreatorExtension
{
public:
    KeyedArchiveChildCreator();
    void ExposeChildren(const std::shared_ptr<DAVA::TArc::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::TArc::PropertyNode>>& children) const override;

private:
    std::unique_ptr<DAVA::ReflectedMeta> elementsMeta;
};

class KeyedArchiveEditorCreator : public DAVA::TArc::EditorComponentExtension
{
public:
    KeyedArchiveEditorCreator(DAVA::TArc::ContextAccessor* accessor);
    std::unique_ptr<DAVA::TArc::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::TArc::PropertyNode>& node) const override;

private:
    DAVA::TArc::ContextAccessor* accessor;
};
