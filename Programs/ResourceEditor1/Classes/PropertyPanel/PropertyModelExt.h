#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>
#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/Core/ContextAccessor.h>

#include <Base/BaseTypes.h>

class SceneEditor2;
class REModifyPropertyExtension : public DAVA::TArc::ModifyExtension
{
public:
    REModifyPropertyExtension(DAVA::TArc::ContextAccessor* accessor);

protected:
    void BeginBatch(const DAVA::String& text, DAVA::uint32 commandCount) override;
    void ProduceCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const DAVA::Any& newValue) override;
    void ProduceCommand(const DAVA::Reflection::Field& object, const DAVA::Any& newValue) override;
    void Exec(std::unique_ptr<DAVA::Command>&& command) override;
    void EndBatch() override;

    SceneEditor2* GetScene() const;

private:
    DAVA::TArc::ContextAccessor* accessor = nullptr;
};

class EntityChildCreator : public DAVA::TArc::ChildCreatorExtension
{
public:
    void ExposeChildren(const std::shared_ptr<DAVA::TArc::PropertyNode>& parent, DAVA::Vector<std::shared_ptr<DAVA::TArc::PropertyNode>>& children) const override;
};

class EntityEditorCreator : public DAVA::TArc::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::TArc::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::TArc::PropertyNode>& node) const override;
};

class ParticleForceCreator : public DAVA::TArc::EditorComponentExtension
{
public:
    std::unique_ptr<DAVA::TArc::BaseComponentValue> GetEditor(const std::shared_ptr<const DAVA::TArc::PropertyNode>& node) const override;
};