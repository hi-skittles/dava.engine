#include "Classes/Selection/Selection.h"
#include "Classes/Selection/SelectionData.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/System/EditorStatisticsSystem.h"
#include "Classes/Commands2/RECommandIDs.h"
#include "Classes/Commands2/Base/RECommandNotificationObject.h"

#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Lod/LodComponent.h>
#include <Reflection/ReflectionRegistrator.h>

#include <Debug/DVAssert.h>

struct TrianglesData
{
    DAVA::Vector<DAVA::uint32> storedTriangles;
    DAVA::Vector<DAVA::uint32> visibleTriangles;
    DAVA::Vector<DAVA::RenderObject*> renderObjects;
};

DAVA_VIRTUAL_REFLECTION_IMPL(RenderStatsSettings)
{
    DAVA::ReflectionRegistrator<RenderStatsSettings>::Begin()[DAVA::M::DisplayName("Render statistics")]
    .ConstructorByPointer()
    .Field("calculatePerFrame", &RenderStatsSettings::calculatePerFrame)[DAVA::M::DisplayName(" Calculate per frame")]
    .End();
}

namespace EditorStatisticsSystemInternal
{
static const DAVA::int32 SIZE_OF_TRIANGLES = DAVA::LodComponent::MAX_LOD_LAYERS + 1;

void EnumerateTriangles(DAVA::RenderObject* renderObject, DAVA::Vector<DAVA::uint32>& triangles, DAVA::Vector<DAVA::uint32>& visibleTriangles)
{
    using namespace DAVA;

    uint32 batchCount = renderObject->GetRenderBatchCount();
    for (uint32 b = 0; b < batchCount; ++b)
    {
        int32 lodIndex = 0;
        int32 switchIndex = 0;

        RenderBatch* rb = renderObject->GetRenderBatch(b, lodIndex, switchIndex);
        lodIndex += 1; //because of non-lod index is -1
        if (lodIndex < 0)
        {
            continue; //means that lod is uninitialized
        }
        DVASSERT(lodIndex <= static_cast<int32>(triangles.size()));

        if (IsPointerToExactClass<RenderBatch>(rb))
        {
            bool batchIsVisible = false;
            uint32 activeBatchCount = renderObject->GetActiveRenderBatchCount();
            for (uint32 a = 0; a < activeBatchCount; ++a)
            {
                if (renderObject->GetActiveRenderBatch(a) == rb)
                {
                    batchIsVisible = true;
                    break;
                }
            }

            PolygonGroup* pg = rb->GetPolygonGroup();
            if (nullptr != pg)
            {
                int32 trianglesCount = pg->GetPrimitiveCount();
                triangles[lodIndex] += trianglesCount;
                if (batchIsVisible)
                {
                    visibleTriangles[lodIndex] += trianglesCount;
                }
            }
        }
    }
}

void EnumerateTriangles(TrianglesData& triangles)
{
    std::fill(triangles.storedTriangles.begin(), triangles.storedTriangles.end(), 0);
    std::fill(triangles.visibleTriangles.begin(), triangles.visibleTriangles.end(), 0);
    for (DAVA::RenderObject* ro : triangles.renderObjects)
    {
        if (ro && (ro->GetType() == DAVA::RenderObject::TYPE_MESH || ro->GetType() == DAVA::RenderObject::TYPE_SPEED_TREE))
        {
            EnumerateTriangles(ro, triangles.storedTriangles, triangles.visibleTriangles);
        }
    }
}

void EnumerateRenderObjectsRecursive(DAVA::Entity* entity, DAVA::Vector<DAVA::RenderObject*>& renderObjects, bool recursive)
{
    if (HasComponent(entity, DAVA::Type::Instance<DAVA::RenderComponent>()))
    {
        DAVA::uint32 componentsCount = entity->GetComponentCount<DAVA::RenderComponent>();
        for (DAVA::uint32 c = 0; c < componentsCount; ++c)
        {
            DAVA::RenderComponent* rc = entity->GetComponent<DAVA::RenderComponent>(c);
            DAVA::RenderObject* ro = rc->GetRenderObject();
            if (ro != nullptr)
            {
                if (std::find(renderObjects.begin(), renderObjects.end(), ro) == renderObjects.end())
                {
                    renderObjects.push_back(ro);
                }
            }
        }
    }

    if (recursive)
    {
        DAVA::uint32 count = entity->GetChildrenCount();
        for (DAVA::uint32 c = 0; c < count; ++c)
        {
            EnumerateRenderObjectsRecursive(entity->GetChild(c), renderObjects, recursive);
        }
    }
}

void EnumerateRenderObjects(const SelectableGroup& group, DAVA::Vector<DAVA::RenderObject*>& renderObjects)
{
    renderObjects.clear();
    if (group.IsEmpty())
        return;

    renderObjects.reserve(group.GetSize());

    CommonInternalSettings* settings = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>();

    for (auto entity : group.ObjectsOfType<DAVA::Entity>())
    {
        EnumerateRenderObjectsRecursive(entity, renderObjects, settings->lodEditorRecursive);
    }
}
}

EditorStatisticsSystem::EditorStatisticsSystem(DAVA::Scene* scene)
    : SceneSystem(scene)
    , binder(new DAVA::TArc::FieldBinder(REGlobal::GetAccessor()))
{
    triangles.resize(eEditorMode::MODE_COUNT);
    for (DAVA::uint32 m = 0; m < eEditorMode::MODE_COUNT; ++m)
    {
        triangles[m].storedTriangles.resize(EditorStatisticsSystemInternal::SIZE_OF_TRIANGLES, 0);
        triangles[m].visibleTriangles.resize(EditorStatisticsSystemInternal::SIZE_OF_TRIANGLES, 0);
    }

    {
        DAVA::TArc::FieldDescriptor descr;
        descr.type = DAVA::ReflectedTypeDB::Get<RenderStatsSettings>();
        descr.fieldName = DAVA::FastName("calculatePerFrame");
        binder->BindField(descr, [this](const DAVA::Any& v) {
            calculatePerFrame = v.Cast<bool>(true);
        });
    }

    {
        DAVA::TArc::FieldDescriptor descr;
        descr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
        descr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
        binder->BindField(descr, [this](const DAVA::Any& v) {
            EmitInvalidateUI(FLAG_TRIANGLES);
        });
    }
}

void EditorStatisticsSystem::RegisterEntity(DAVA::Entity* entity)
{
    if (HasComponent(entity, DAVA::Type::Instance<DAVA::RenderComponent>()) || HasComponent(entity, DAVA::Type::Instance<DAVA::LodComponent>()))
    {
        EmitInvalidateUI(FLAG_TRIANGLES);
    }
}

void EditorStatisticsSystem::UnregisterEntity(DAVA::Entity* entity)
{
    if (HasComponent(entity, DAVA::Type::Instance<DAVA::RenderComponent>()) || HasComponent(entity, DAVA::Type::Instance<DAVA::LodComponent>()))
    {
        EmitInvalidateUI(FLAG_TRIANGLES);
    }
}

void EditorStatisticsSystem::RegisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    const DAVA::Type* type = component->GetType();
    if (type->Is<DAVA::RenderComponent>() || type->Is<DAVA::LodComponent>())
    {
        EmitInvalidateUI(FLAG_TRIANGLES);
    }
}

void EditorStatisticsSystem::UnregisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    const DAVA::Type* type = component->GetType();
    if (type->Is<DAVA::RenderComponent>() || type->Is<DAVA::LodComponent>())
    {
        EmitInvalidateUI(FLAG_TRIANGLES);
    }
}

void EditorStatisticsSystem::PrepareForRemove()
{
}

const DAVA::Vector<DAVA::uint32>& EditorStatisticsSystem::GetTriangles(eEditorMode mode, bool allTriangles)
{
    if (calculatePerFrame == false && initialized == true)
    {
        CalculateTriangles();
    }

    if (allTriangles)
    {
        return triangles[mode].storedTriangles;
    }

    return triangles[mode].visibleTriangles;
}

void EditorStatisticsSystem::Process(DAVA::float32 timeElapsed)
{
    initialized = true;
    if (calculatePerFrame == true)
    {
        CalculateTriangles();
    }
    DispatchSignals();
}

void EditorStatisticsSystem::ClipSelection(DAVA::Camera* camera, DAVA::Vector<DAVA::RenderObject*>& selection,
                                           DAVA::Vector<DAVA::RenderObject*>& visibilityArray, DAVA::uint32 visibilityCriteria)
{
    DAVA::Frustum* frustum = camera->GetFrustum();
    DAVA::uint32 size = static_cast<DAVA::uint32>(selection.size());
    for (DAVA::uint32 pos = 0; pos < size; ++pos)
    {
        DAVA::RenderObject* node = selection[pos];
        if ((node->GetFlags() & visibilityCriteria) != visibilityCriteria)
        {
            continue;
        }
        if ((DAVA::RenderObject::ALWAYS_CLIPPING_VISIBLE & node->GetFlags()) ||
            frustum->IsInside(node->GetWorldBoundingBox()))
        {
            visibilityArray.push_back(node);
        }
    }
}

void EditorStatisticsSystem::CalculateTriangles()
{
    auto CalculateTrianglesForMode = [this](eEditorMode mode)
    {
        DAVA::Vector<DAVA::uint32> storedTriangles = triangles[mode].storedTriangles;
        DAVA::Vector<DAVA::uint32> visibleTriangles = triangles[mode].visibleTriangles;

        EditorStatisticsSystemInternal::EnumerateTriangles(triangles[mode]);
        if (triangles[mode].storedTriangles != storedTriangles || triangles[mode].visibleTriangles != visibleTriangles)
        {
            EmitInvalidateUI(FLAG_TRIANGLES);
        }
    };

    SceneEditor2* editorScene = static_cast<SceneEditor2*>(GetScene());

    //Scene
    triangles[eEditorMode::MODE_ALL_SCENE].renderObjects.clear();

    //Selection
    triangles[eEditorMode::MODE_SELECTION].renderObjects.clear();
    const SelectableGroup& selection = Selection::GetSelection();
    DAVA::Vector<DAVA::RenderObject*> selectedObjects;
    EditorStatisticsSystemInternal::EnumerateRenderObjects(selection, selectedObjects);

    // Clip objects
    DAVA::Camera* drawCamera = editorScene->GetDrawCamera();
    if (drawCamera != nullptr)
    {
        DAVA::uint32 currVisibilityCriteria = DAVA::RenderObject::CLIPPING_VISIBILITY_CRITERIA;
        editorScene->renderSystem->GetRenderHierarchy()->Clip(drawCamera, triangles[eEditorMode::MODE_ALL_SCENE].renderObjects, currVisibilityCriteria);
        ClipSelection(drawCamera, selectedObjects, triangles[eEditorMode::MODE_SELECTION].renderObjects, currVisibilityCriteria);
    }
    CalculateTrianglesForMode(eEditorMode::MODE_ALL_SCENE);
    CalculateTrianglesForMode(eEditorMode::MODE_SELECTION);
}

void EditorStatisticsSystem::EmitInvalidateUI(DAVA::uint32 flags)
{
    invalidateUIflag = flags;
}

void EditorStatisticsSystem::DispatchSignals()
{
    if (invalidateUIflag == FLAG_NONE)
    {
        return;
    }

    for (auto& d : uiDelegates)
    {
        if (invalidateUIflag & FLAG_TRIANGLES)
        {
            d->UpdateTrianglesUI(this);
        }
    }

    invalidateUIflag = FLAG_NONE;
}

void EditorStatisticsSystem::ProcessCommand(const RECommandNotificationObject& commandNotification)
{
    static DAVA::Vector<DAVA::uint32> commandIDs = {
        CMDID_DELETE_RENDER_BATCH,
        CMDID_LOD_COPY_LAST_LOD,
        CMDID_LOD_CREATE_PLANE,
        CMDID_LOD_DELETE
    };

    if (commandNotification.MatchCommandIDs(commandIDs))
    {
        EmitInvalidateUI(FLAG_TRIANGLES);
    }
}

void EditorStatisticsSystem::AddDelegate(EditorStatisticsSystemUIDelegate* uiDelegate)
{
    DVASSERT(uiDelegate != nullptr);

    uiDelegates.push_back(uiDelegate);
    if (uiDelegate != nullptr)
    {
        uiDelegate->UpdateTrianglesUI(this);
    }
}

void EditorStatisticsSystem::RemoveDelegate(EditorStatisticsSystemUIDelegate* uiDelegate)
{
    DVASSERT(uiDelegate != nullptr);

    DAVA::FindAndRemoveExchangingWithLast(uiDelegates, uiDelegate);
}
