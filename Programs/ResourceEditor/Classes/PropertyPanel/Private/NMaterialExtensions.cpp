#include "Classes/PropertyPanel/NMaterialExtensions.h"

#include <REPlatform/Global/GlobalOperations.h>

#include <TArc/Core/OperationInvoker.h>
#include <TArc/Utils/Utils.h>

#include <Render/Material/NMaterial.h>

namespace NMaterialExtensionsDetail
{
using namespace DAVA;
class OpenMaterialEditorProducer : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const override;
    Info GetInfo() const override;
    bool OnlyForSingleSelection() const override;
    std::unique_ptr<Command> CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const override;
};

bool OpenMaterialEditorProducer::IsApplyable(const std::shared_ptr<DAVA::PropertyNode>& node) const
{
    return node->field.ref.GetValueType() == Type::Instance<NMaterial*>();
}

M::CommandProducer::Info OpenMaterialEditorProducer::GetInfo() const
{
    Info info;
    info.icon = DAVA::SharedIcon(":/QtIcons/3d.png");
    info.tooltip = "Edit material";
    return info;
}

bool OpenMaterialEditorProducer::OnlyForSingleSelection() const
{
    return true;
}

std::unique_ptr<Command> OpenMaterialEditorProducer::CreateCommand(const std::shared_ptr<DAVA::PropertyNode>& node, const Params& params) const
{
    NMaterial* material = *node->field.ref.GetValueObject().GetPtr<NMaterial*>();
    params.invoker->Invoke(DAVA::ShowMaterial.ID, material);
    return std::unique_ptr<Command>();
}
}

DAVA::M::CommandProducerHolder CreateNMaterialCommandProducer()
{
    using namespace NMaterialExtensionsDetail;
    DAVA::M::CommandProducerHolder holder;
    holder.AddCommandProducer(std::make_unique<OpenMaterialEditorProducer>());
    return holder;
}
