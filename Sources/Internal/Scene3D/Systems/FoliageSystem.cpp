#include "Scene3D/Systems/FoliageSystem.h"

#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/Vegetation/VegetationRenderObject.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Systems/WindSystem.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"

namespace DAVA
{
static const int16 MAX_ANIMATED_CELL_WIDTH = 2;
static const int16 MIN_ANIMATED_CELL_WIDTH = 1;

FoliageSystem::FoliageSystem(Scene* scene)
    : SceneSystem(scene)
{
}

FoliageSystem::~FoliageSystem()
{
    SafeRelease(landscapeEntity);
    for (auto fe : foliageEntities)
    {
        SafeRelease(fe);
    }
}

void FoliageSystem::AddEntity(Entity* entity)
{
    Landscape* landscapeRO = GetLandscape(entity);
    if (landscapeRO && (entity != landscapeEntity))
    {
        SafeRelease(landscapeEntity);
        landscapeEntity = SafeRetain(entity);
        landscapeRO->SetFoliageSystem(this);
        SyncFoliageWithLandscape();
    }

    VegetationRenderObject* vegetationRO = GetVegetation(entity);
    if (vegetationRO != nullptr)
    {
        if (std::find(foliageEntities.begin(), foliageEntities.end(), entity) == foliageEntities.end())
        {
            foliageEntities.push_back(SafeRetain(entity));
            SyncFoliageWithLandscape();
        }
    }
}

void FoliageSystem::RemoveEntity(Entity* entity)
{
    auto i = std::find(foliageEntities.begin(), foliageEntities.end(), entity);
    if (i != foliageEntities.end())
    {
        SafeRelease(*i);
        foliageEntities.erase(i);
    }

    if (entity == landscapeEntity)
    {
        SafeRelease(landscapeEntity);
    }
}

void FoliageSystem::PrepareForRemove()
{
    for (Entity* e : foliageEntities)
    {
        SafeRelease(e);
    }
    SafeRelease(landscapeEntity);
    foliageEntities.clear();
}

void FoliageSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_FOLIAGE_SYSTEM);

    if (GetScene()->GetRenderSystem()->GetMainCamera() == nullptr)
    {
        return;
    }

    for (auto foliageEntity : foliageEntities)
    {
        VegetationRenderObject* vegetationRO = GetVegetation(foliageEntity);
        if (vegetationRO && vegetationRO->ReadyToRender())
        {
            ProcessVegetationRenderObject(vegetationRO, timeElapsed);
        }
    }
}

void FoliageSystem::ProcessVegetationRenderObject(VegetationRenderObject* vegetationRO, float32 timeElapsed)
{
    WindSystem* windSystem = GetScene()->windSystem;

    Camera* camera = GetScene()->GetRenderSystem()->GetMainCamera();
    Vector<AbstractQuadTreeNode<VegetationSpatialData>*>& visibleCells = vegetationRO->BuildVisibleCellList(camera);
    uint32 cellsCount = static_cast<uint32>(visibleCells.size());

    Set<AbstractQuadTreeNode<VegetationSpatialData>*> updatableCells;
    for (uint32 i = 0; i < cellsCount; ++i)
    {
        AbstractQuadTreeNode<VegetationSpatialData>* cell = visibleCells[i];
        if (cell->data.width <= MAX_ANIMATED_CELL_WIDTH)
        {
            bool isMinAnimatedLod = (MIN_ANIMATED_CELL_WIDTH == cell->data.width);
            if (isMinAnimatedLod)
            {
                updatableCells.insert(cell->parent);
            }
            else
            {
                updatableCells.insert(cell);
            }
        }
    }

    Vector4 layersAnimationSpring = vegetationRO->GetLayersAnimationSpring();
    const Vector4& layerAnimationDrag = vegetationRO->GetLayerAnimationDragCoefficient();
    for (auto cell : updatableCells)
    {
        VegetationSpatialData& cellData = cell->data;

        const Vector3& min = cellData.bbox.min;
        const Vector3& max = cellData.bbox.max;

        Vector3 cellPos[4] = {
            Vector3(min.x, min.y, max.z),
            Vector3(min.x, max.y, max.z),
            Vector3(max.x, min.y, max.z),
            Vector3(max.x, max.y, max.z)
        };

        for (uint32 layerIndex = 0; layerIndex < 4; ++layerIndex)
        {
            Vector3 windVec = windSystem->GetWind(cellPos[layerIndex]);
            Vector2 windVec2D(windVec.x, windVec.y);
            Vector2& offset = cellData.animationOffset[layerIndex];
            Vector2& velocity = cellData.animationVelocity[layerIndex];
            velocity += (windVec2D - layersAnimationSpring.data[layerIndex] * offset -
                         layerAnimationDrag.data[layerIndex] * velocity * velocity.Length()) *
            timeElapsed;
            offset += velocity * timeElapsed;
        }

        if (cell->children != nullptr)
        {
            for (uint32 childIndex = 0; childIndex < 4; ++childIndex)
            {
                cell->children[childIndex]->data.animationOffset[0] = cellData.animationOffset[0];
                cell->children[childIndex]->data.animationOffset[1] = cellData.animationOffset[1];
                cell->children[childIndex]->data.animationOffset[2] = cellData.animationOffset[2];
                cell->children[childIndex]->data.animationOffset[3] = cellData.animationOffset[3];
                cell->children[childIndex]->data.animationVelocity[0] = cellData.animationVelocity[0];
                cell->children[childIndex]->data.animationVelocity[1] = cellData.animationVelocity[1];
                cell->children[childIndex]->data.animationVelocity[2] = cellData.animationVelocity[2];
                cell->children[childIndex]->data.animationVelocity[3] = cellData.animationVelocity[3];
            }
        }
    }
}

void FoliageSystem::SyncFoliageWithLandscape()
{
    if (landscapeEntity == nullptr)
        return;

    for (auto foliageEntity : foliageEntities)
    {
        if (!QualitySettingsSystem::Instance()->IsQualityVisible(foliageEntity))
            continue;
        Landscape* landscapeRO = GetLandscape(landscapeEntity);
        VegetationRenderObject* vegetationRO = GetVegetation(foliageEntity);
        vegetationRO->SetHeightmap(landscapeRO->GetHeightmap());
        vegetationRO->SetHeightmapPath(landscapeRO->GetHeightmapPathname());
        vegetationRO->SetWorldSize(Vector3(landscapeRO->GetLandscapeSize(), landscapeRO->GetLandscapeSize(), landscapeRO->GetLandscapeHeight()));
    }
}

void FoliageSystem::SetPerturbation(const Vector3& point, const Vector3& force, float32 distance)
{
    for (auto foliageEntity : foliageEntities)
    {
        VegetationRenderObject* vegetationRO = GetVegetation(foliageEntity);
        if (vegetationRO != nullptr)
        {
            vegetationRO->SetPerturbation(point, force, distance);
        }
    }
}

void FoliageSystem::SetFoliageVisible(bool show)
{
    for (auto foliageEntity : foliageEntities)
    {
        VegetationRenderObject* vegetationRO = GetVegetation(foliageEntity);
        if (nullptr != vegetationRO)
        {
            vegetationRO->SetVegetationVisible(show);
        }
    }
}

bool FoliageSystem::IsFoliageVisible() const
{
    for (auto foliageEntity : foliageEntities)
    {
        VegetationRenderObject* vegetationRO = GetVegetation(foliageEntity);
        if ((vegetationRO != nullptr) && vegetationRO->GetVegetationVisible())
            return true;
    }
    return false;
}

void FoliageSystem::DebugDrawVegetation()
{
    for (auto foliageEntity : foliageEntities)
    {
        VegetationRenderObject* vegetationRO = GetVegetation(foliageEntity);
        if (nullptr != vegetationRO)
        {
            vegetationRO->DebugDrawVisibleNodes(GetScene()->GetRenderSystem()->GetDebugDrawer());
        }
    }
}

void FoliageSystem::CollectFoliageMaterials(Set<NMaterial*>& materials)
{
    for (auto foliageEntity : foliageEntities)
    {
        Set<DataNode*> dataNodes;
        foliageEntity->GetDataNodes(dataNodes);

        Set<DataNode*>::iterator it = dataNodes.begin();
        Set<DataNode*>::iterator itEnd = dataNodes.end();
        for (; it != itEnd; ++it)
        {
            NMaterial* material = dynamic_cast<NMaterial*>(*it);
            if (material)
            {
                materials.insert(material);
            }
        }
    }
}
};