#pragma once

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>

#include <Reflection/ReflectedMeta.h>

class KeyedArchiveChildCreator : public DAVA::ChildCreatorExtension
{
public:
    KeyedArchiveChildCreator();
    void ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const override;

private:
    std::unique_ptr<DAVA::ReflectedMeta> elementsMeta;
};

class KeyedArchiveEditorCreator : public DAVA::EditorComponentExtension
{
public:
    KeyedArchiveEditorCreator(DAVA::ContextAccessor* accessor);
    std::unique_ptr<DAVA::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const override;

private:
    DAVA::ContextAccessor* accessor;
};
