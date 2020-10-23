#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Core/ContextAccessor.h>

#include <Base/BaseTypes.h>

namespace DAVA
{
class SceneEditor2;
} // namespace DAVA

class REModifyPropertyExtension : public DAVA::ModifyExtension
{
public:
    REModifyPropertyExtension(DAVA::ContextAccessor* accessor);

protected:
    void BeginBatch(const DAVA::String& text, DAVA::uint32 commandCount) override;
    void ProduceCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const DAVA::Any& newValue) override;
    void ProduceCommand(const DAVA::Reflection::Field& object, const DAVA::Any& newValue) override;
    void Exec(std::unique_ptr<DAVA::Command>&& command) override;
    void EndBatch() override;

    DAVA::SceneEditor2* GetScene() const;

private:
    DAVA::ContextAccessor* accessor = nullptr;
};

class EntityChildCreator : public DAVA::ChildCreatorExtension
{
public:
    void ExposeChildren(const std::shared_ptr<DAVA::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::PropertyNode>>& children) const override;
};

class EntityEditorCreator : public DAVA::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const override;
};

class ParticleForceCreator : public DAVA::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::PropertyNode>& node) const override;
};