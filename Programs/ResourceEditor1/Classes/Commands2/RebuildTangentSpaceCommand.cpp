#include "RebuildTangentSpaceCommand.h"
#include "Commands2/RECommandIDs.h"
#include "Render/3D/MeshUtils.h"

RebuildTangentSpaceCommand::RebuildTangentSpaceCommand(DAVA::RenderBatch* _renderBatch, bool _computeBinormal)
    : RECommand(CMDID_REBUILD_TANGENT_SPACE, "Rebuild Tangent Space")
    , renderBatch(_renderBatch)
    , computeBinormal(_computeBinormal)
{
    DVASSERT(renderBatch);
    DAVA::PolygonGroup* srcGroup = renderBatch->GetPolygonGroup();
    DVASSERT(srcGroup);
    originalGroup = new DAVA::PolygonGroup();
    DAVA::MeshUtils::CopyGroupData(srcGroup, originalGroup);
}

RebuildTangentSpaceCommand::~RebuildTangentSpaceCommand()
{
    SafeRelease(originalGroup);
}

void RebuildTangentSpaceCommand::Redo()
{
    DAVA::MeshUtils::RebuildMeshTangentSpace(renderBatch->GetPolygonGroup(), computeBinormal);
}

void RebuildTangentSpaceCommand::Undo()
{
    DAVA::MeshUtils::CopyGroupData(originalGroup, renderBatch->GetPolygonGroup());
}
