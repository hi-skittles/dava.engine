#pragma once

#include "Base/BaseTypes.h"
#include "RHI/rhi_Public.h"
#include "RHI/rhi_Type.h"

namespace DAVA
{
namespace VisibilityQueryResults
{
enum eQueryIndex
{
    QUERY_INDEX_LAYER_OPAQUE = 0,
    QUERY_INDEX_LAYER_AFTER_OPAQUE,
    QUERY_INDEX_LAYER_ALPHA_TEST_LAYER,
    QUERY_INDEX_LAYER_WATER,
    QUERY_INDEX_LAYER_TRANSLUCENT,
    QUERY_INDEX_LAYER_AFTER_TRANSLUCENT,
    QUERY_INDEX_LAYER_SHADOW_VOLUME,
    QUERY_INDEX_LAYER_VEGETATION,
    QUERY_INDEX_LAYER_DEBUG_DRAW,
    QUERY_INDEX_UI,
    QUERY_INDEX_ALPHABLEND,

    QUERY_INDEX_COUNT,
};

rhi::HQueryBuffer GetQueryBuffer();

uint32 GetResult(eQueryIndex index);
const FastName& GetQueryIndexName(eQueryIndex index);

void EndFrame();
void Cleanup();

} //ns VisibilityResults
} //ns DAVA
