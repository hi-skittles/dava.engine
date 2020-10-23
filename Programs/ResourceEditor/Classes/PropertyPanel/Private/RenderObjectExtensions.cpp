#include "Classes/PropertyPanel/RenderObjectExtensions.h"

#include <REPlatform/Commands/CloneLastBatchCommand.h>
#include <REPlatform/Commands/ConvertToBillboardCommand.h>
#include <REPlatform/Commands/ConvertToShadowCommand.h>
#include <REPlatform/Commands/DeleteRenderBatchCommand.h>
#include <REPlatform/Commands/RebuildTangentSpaceCommand.h>
#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/Deprecated/SceneValidator.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Utils/Utils.h>

#include <Scene3D/Components/RenderComponent.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Render/Highlevel/RenderObject.h>
#include <Debug/DVAssert.h>

namespace RenderObjectExtensionsDetail
{
class BillboardCommandProducer : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const override;
    Info GetInfo() const override;
    void CreateCache(DAVA::ContextAccessor* accessor) override;
    void ClearCache() override;
    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const override;

private:
    DAVA::UnorderedMap<DAVA::RenderObject*, DAVA::Entity*> cache;
};

bool BillboardCommandProducer::IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const
{
    using namespace DAVA;
    RenderObject* renderObject = *node->field.ref.GetValueObject().GetPtr<RenderObject*>();
    RenderObject::eType type = renderObject->GetType();
    return type == RenderObject::TYPE_MESH || type == RenderObject::TYPE_RENDEROBJECT;
}

std::unique_ptr<DAVA::Command> BillboardCommandProducer::CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const
{
    using namespace DAVA;
    RenderObject* renderObject = *node->field.ref.GetValueObject().GetPtr<RenderObject*>();
    DAVA::RenderObject::eType type = renderObject->GetType();
    DVASSERT(type == DAVA::RenderObject::TYPE_MESH || type == DAVA::RenderObject::TYPE_RENDEROBJECT);

    auto iter = cache.find(renderObject);
    DVASSERT(iter != cache.end());

    return std::make_unique<ConvertToBillboardCommand>(renderObject, iter->second);
}

void BillboardCommandProducer::CreateCache(DAVA::ContextAccessor* accessor)
{
    using namespace DAVA;

    DAVA::DataContext* ctx = accessor->GetActiveContext();
    DVASSERT(ctx != nullptr);
    SelectionData* data = ctx->GetData<SelectionData>();
    SelectableGroup selection = data->GetMutableSelection();
    for (Selectable& obj : selection.GetMutableContent())
    {
        DAVA::Entity* entity = obj.AsEntity();
        if (entity != nullptr)
        {
            RenderObject* renderObject = GetRenderObject(entity);
            if (renderObject != nullptr)
            {
                cache.emplace(renderObject, entity);
            }
        }
    }
}

void BillboardCommandProducer::ClearCache()
{
    cache.clear();
}

DAVA::M::CommandProducer::Info BillboardCommandProducer::GetInfo() const
{
    Info info;
    info.icon = DAVA::SharedIcon(":/QtIcons/sphere.png");
    info.tooltip = QStringLiteral("Make billboard");
    info.description = "Convert to billboard";
    return info;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Fix lods and switches                                                    //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FixLodsAndSwitches : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const override;
    Info GetInfo() const override;
    bool OnlyForSingleSelection() const override;
    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const override;
};

bool FixLodsAndSwitches::IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const
{
    using namespace DAVA;
    RenderObject* renderObject = *node->field.ref.GetValueObject().GetPtr<RenderObject*>();
    return SceneValidator::IsObjectHasDifferentLODsCount(renderObject);
}

DAVA::M::CommandProducer::Info FixLodsAndSwitches::GetInfo() const
{
    Info info;
    info.icon = DAVA::SharedIcon(":/QtIcons/clone_batches.png");
    info.tooltip = QStringLiteral("Clone batches for LODs correction");
    info.description = "Clone Last Batch";
    return info;
}

bool FixLodsAndSwitches::OnlyForSingleSelection() const
{
    return true;
}

std::unique_ptr<DAVA::Command> FixLodsAndSwitches::CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const
{
    using namespace DAVA;
    RenderObject* renderObject = *node->field.ref.GetValueObject().GetPtr<RenderObject*>();
    return std::make_unique<CloneLastBatchCommand>(renderObject);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     Remove render batch                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RemoveRenderBatch : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const override;
    Info GetInfo() const override;
    bool OnlyForSingleSelection() const override;
    void CreateCache(DAVA::ContextAccessor* accessor) override;
    void ClearCache() override;
    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const override;

private:
    DAVA::UnorderedMap<DAVA::RenderBatch*, std::pair<DAVA::Entity*, DAVA::uint32>> cache;
};

bool RemoveRenderBatch::IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const
{
    return true;
}

DAVA::M::CommandProducer::Info RemoveRenderBatch::GetInfo() const
{
    Info info;
    info.icon = DAVA::SharedIcon(":/QtIcons/remove.png");
    info.tooltip = QStringLiteral("Delete render batch");
    info.description = "Render batch deletion";
    return info;
}

bool RemoveRenderBatch::OnlyForSingleSelection() const
{
    return true;
}

void RemoveRenderBatch::CreateCache(DAVA::ContextAccessor* accessor)
{
    using namespace DAVA;

    DAVA::DataContext* ctx = accessor->GetActiveContext();
    DVASSERT(ctx != nullptr);
    SelectionData* data = ctx->GetData<SelectionData>();
    SelectableGroup selection = data->GetMutableSelection();
    for (Selectable& obj : selection.GetMutableContent())
    {
        DAVA::Entity* entity = obj.AsEntity();
        if (entity != nullptr)
        {
            RenderObject* renderObject = GetRenderObject(entity);
            if (renderObject != nullptr)
            {
                for (uint32 i = 0; i < renderObject->GetRenderBatchCount(); ++i)
                {
                    cache.emplace(renderObject->GetRenderBatch(i), std::make_pair(entity, i));
                }
            }
        }
    }
}

void RemoveRenderBatch::ClearCache()
{
    cache.clear();
}

std::unique_ptr<DAVA::Command> RemoveRenderBatch::CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const
{
    DAVA::RenderBatch* batch = *node->field.ref.GetValueObject().GetPtr<DAVA::RenderBatch*>();
    auto iter = cache.find(batch);
    DVASSERT(iter != cache.end());
    DAVA::RenderObject* renderOject = batch->GetRenderObject();
    if (renderOject->GetRenderBatchCount() < 2)
    {
        return nullptr;
    }
    return std::make_unique<DAVA::DeleteRenderBatchCommand>(iter->second.first, renderOject, iter->second.second);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    Convert to shadow                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ConvertToShadow : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const override;
    Info GetInfo() const override;
    bool OnlyForSingleSelection() const override;
    void CreateCache(DAVA::ContextAccessor* accessor) override;
    void ClearCache() override;
    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const override;

private:
    DAVA::UnorderedMap<DAVA::RenderBatch*, DAVA::Entity*> cache;
};

bool ConvertToShadow::IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const
{
    DAVA::RenderBatch* batch = *node->field.ref.GetValueObject().GetPtr<DAVA::RenderBatch*>();
    return DAVA::ConvertToShadowCommand::CanConvertBatchToShadow(batch);
}

DAVA::M::CommandProducer::Info ConvertToShadow::GetInfo() const
{
    Info info;
    info.icon = DAVA::SharedIcon(":/QtIcons/shadow.png");
    info.tooltip = QStringLiteral("Convert To ShadowVolume");
    info.description = "ConvertToShadow batch";
    return info;
}

bool ConvertToShadow::OnlyForSingleSelection() const
{
    return true;
}

void ConvertToShadow::CreateCache(DAVA::ContextAccessor* accessor)
{
    using namespace DAVA;

    DAVA::DataContext* ctx = accessor->GetActiveContext();
    DVASSERT(ctx != nullptr);
    SelectionData* data = ctx->GetData<SelectionData>();
    SelectableGroup selection = data->GetMutableSelection();
    for (Selectable& obj : selection.GetMutableContent())
    {
        DAVA::Entity* entity = obj.AsEntity();
        if (entity != nullptr)
        {
            RenderObject* renderObject = GetRenderObject(entity);
            if (renderObject != nullptr)
            {
                for (uint32 i = 0; i < renderObject->GetRenderBatchCount(); ++i)
                {
                    cache.emplace(renderObject->GetRenderBatch(i), entity);
                }
            }
        }
    }
}

void ConvertToShadow::ClearCache()
{
    cache.clear();
}

std::unique_ptr<DAVA::Command> ConvertToShadow::CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const
{
    DAVA::RenderBatch* batch = *node->field.ref.GetValueObject().GetPtr<DAVA::RenderBatch*>();
    auto iter = cache.find(batch);
    DVASSERT(iter != cache.end());
    return std::make_unique<DAVA::ConvertToShadowCommand>(iter->second, batch);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     Rebuild tangent                                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class RebuildTangentSpace : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const override;
    Info GetInfo() const override;
    bool OnlyForSingleSelection() const override;
    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const override;

private:
    DAVA::UnorderedMap<DAVA::RenderBatch*, DAVA::Entity*> cache;
};

bool RebuildTangentSpace::IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const
{
    DAVA::RenderBatch* batch = *node->field.ref.GetValueObject().GetPtr<DAVA::RenderBatch*>();
    DAVA::PolygonGroup* group = batch->GetPolygonGroup();
    if (group == nullptr)
    {
        return false;
    }
    bool isRebuildTsEnabled = true;
    const DAVA::int32 requiredVertexFormat = (DAVA::EVF_TEXCOORD0 | DAVA::EVF_NORMAL);
    isRebuildTsEnabled &= (group->GetPrimitiveType() == rhi::PRIMITIVE_TRIANGLELIST);
    isRebuildTsEnabled &= ((group->GetFormat() & requiredVertexFormat) == requiredVertexFormat);

    return isRebuildTsEnabled;
}

DAVA::M::CommandProducer::Info RebuildTangentSpace::GetInfo() const
{
    Info info;
    info.icon = DAVA::SharedIcon(":/QtIcons/external.png");
    info.tooltip = QStringLiteral("Rebuild tangent space");
    info.description = "ConvertToShadow batch";
    return info;
}

bool RebuildTangentSpace::OnlyForSingleSelection() const
{
    return true;
}

std::unique_ptr<DAVA::Command> RebuildTangentSpace::CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const
{
    DAVA::RenderBatch* batch = *node->field.ref.GetValueObject().GetPtr<DAVA::RenderBatch*>();
    return std::make_unique<DAVA::RebuildTangentSpaceCommand>(batch, true);
}
}

DAVA::M::CommandProducerHolder CreateRenderObjectCommandProducer()
{
    using namespace RenderObjectExtensionsDetail;

    DAVA::M::CommandProducerHolder holder;
    holder.AddCommandProducer(std::make_shared<BillboardCommandProducer>());
    holder.AddCommandProducer(std::make_shared<FixLodsAndSwitches>());
    return holder;
}

DAVA::M::CommandProducerHolder CreateRenderBatchCommandProducer()
{
    using namespace RenderObjectExtensionsDetail;

    DAVA::M::CommandProducerHolder holder;
    holder.AddCommandProducer(std::make_shared<RemoveRenderBatch>());
    holder.AddCommandProducer(std::make_shared<ConvertToShadow>());
    holder.AddCommandProducer(std::make_shared<RebuildTangentSpace>());
    return holder;
}
