#include "Render/Material/NMaterial.h"
#include "Render/Highlevel/Vegetation/VegetationRenderData.h"

namespace DAVA
{
VegetationRenderData::VegetationRenderData()
    : material(NULL)
{
}

VegetationRenderData::~VegetationRenderData()
{
    SafeRelease(material);
}

void VegetationRenderData::ReleaseRenderData()
{
    vertexData.clear();
    indexData.clear();
}
}
