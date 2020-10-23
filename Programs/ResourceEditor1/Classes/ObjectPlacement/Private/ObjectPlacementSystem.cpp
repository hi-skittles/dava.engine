#include "Classes/ObjectPlacement/Private/ObjectPlacementSystem.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/System/ModifSystem.h"
#include "Classes/Selection/Selection.h"

#include <Scene3D/Systems/LandscapeSystem.h>

ObjectPlacementSystem::ObjectPlacementSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
{
    SceneEditor2* editorScene = static_cast<SceneEditor2*>(scene);
    modificationSystem = editorScene->modifSystem;
    DVASSERT(modificationSystem != nullptr);
    renderSystem = editorScene->renderSystem;
    DVASSERT(renderSystem != nullptr);
    landscapeSystem = editorScene->landscapeSystem;
    DVASSERT(landscapeSystem != nullptr);
}

bool ObjectPlacementSystem::GetSnapToLandscape() const
{
    return snapToLandscape;
}

void ObjectPlacementSystem::SetSnapToLandscape(bool newSnapToLandscape)
{
    const DAVA::Vector<DAVA::Entity*>& landscapes = landscapeSystem->GetLandscapeEntities();
    if (landscapes.empty())
    {
        DAVA::Logger::Error(ResourceEditor::NO_LANDSCAPE_ERROR_MESSAGE.c_str());
        return;
    }
    snapToLandscape = newSnapToLandscape;
    modificationSystem->SetLandscapeSnap(snapToLandscape);
}

void ObjectPlacementSystem::PlaceOnLandscape() const
{
    const DAVA::Vector<DAVA::Entity*>& landscapes = landscapeSystem->GetLandscapeEntities();
    if (landscapes.empty())
    {
        DAVA::Logger::Error(ResourceEditor::NO_LANDSCAPE_ERROR_MESSAGE.c_str());
        return;
    }

    SelectableGroup selection = Selection::GetSelection();

    selection.RemoveIf([this](const Selectable& obj) {
        return obj.AsEntity()->GetLocked();
    });

    modificationSystem->PlaceOnLandscape(selection);
}

void ObjectPlacementSystem::RemoveEntity(DAVA::Entity* entity)
{
    needCheckLandscapes = true;
}

void ObjectPlacementSystem::Process(DAVA::float32 time)
{
    if (needCheckLandscapes == true)
    {
        const DAVA::Vector<DAVA::Entity*>& landscapes = landscapeSystem->GetLandscapeEntities();
        if (landscapes.empty())
        {
            snapToLandscape = false;
            modificationSystem->SetLandscapeSnap(false);
        }
        needCheckLandscapes = false;
    }
}

void ObjectPlacementSystem::PlaceAndAlign() const
{
    using namespace DAVA;

    SelectableGroup entities = Selection::GetSelection();

    entities.RemoveIf([this](const Selectable& obj) {
        return obj.AsEntity()->GetLocked();
    });

    Vector<EntityToModify> modifEntities = CreateEntityToModifyVector(entities);
    if (modifEntities.empty())
        return;

    for (EntityToModify& etm : modifEntities)
    {
        Vector3 translationVector = Vector3(0.0f, 0.0f, 0.0f);
        Vector3 collisionNormal;
        bool hitObject = false;
        bool hitLandscape = false;

        Vector3 originalPos = etm.originalTransform.GetTranslationVector()
        * etm.originalParentWorldTransform;
        Ray3 ray(originalPos, Vector3(0.0f, 0.0f, -1.0f));
        RayTraceCollision collision;

        Vector<RenderObject*> selectedObjects;
        for (const Selectable& item : entities.GetContent())
        {
            auto addRo = [&selectedObjects](Entity* entity)
            {
                RenderObject* ro = GetRenderObject(entity);
                if (ro != nullptr)
                {
                    selectedObjects.push_back(ro);
                }
            };

            addRo(item.AsEntity());

            Vector<Entity*> children;
            item.AsEntity()->GetChildEntitiesWithComponent(children, DAVA::Type::Instance<DAVA::RenderComponent>());
            for (Entity* entity : children)
            {
                addRo(entity);
            }
        }

        hitObject = renderSystem->GetRenderHierarchy()->RayTrace(ray, collision, selectedObjects);
        if (hitObject)
        {
            GetObjectCollisionMatrixAndNormal(collision, translationVector, collisionNormal);
        }
        else
        {
            Vector3 landscapeCollision;
            DAVA::Vector<DAVA::Landscape*> landscapes = landscapeSystem->GetLandscapeObjects();
            for (Landscape* landscape : landscapes)
            {
                hitLandscape = landscape->PlacePoint(originalPos, landscapeCollision, &collisionNormal);
                if (hitLandscape)
                {
                    translationVector.z = landscapeCollision.z - originalPos.z;
                }
            }
        }

        if (!(hitLandscape || hitObject))
            continue;

        Vector3 currentUp = Vector3(0.0f, 0.0f, 1.0f);
        Matrix4 originalRotation = etm.originalTransform;
        originalRotation.SetTranslationVector(Vector3(0.0f, 0.0f, 0.0f));
        etm.originalParentWorldTransform.SetTranslationVector(Vector3(0.0f, 0.0f, 0.0f));
        currentUp = currentUp * originalRotation * etm.originalParentWorldTransform;

        Quaternion rotationQuaternion;
        rotationQuaternion = Quaternion::MakeRotation(currentUp, collisionNormal);
        Matrix4 rotation = rotationQuaternion.GetMatrix();

        Matrix4 translation;
        translation.SetTranslationVector(translationVector * etm.inversedParentWorldTransform);

        etm.object.SetLocalTransform(etm.originalTransform
                                     * etm.toLocalZero
                                     * etm.inversedParentWorldTransform
                                     * rotation
                                     * etm.originalParentWorldTransform
                                     * etm.fromLocalZero
                                     * translation
                                     );
    }
    ApplyModificationToScene(GetScene(), modifEntities);
}

void ObjectPlacementSystem::GetObjectCollisionMatrixAndNormal(DAVA::RayTraceCollision& collision,
                                                              DAVA::Vector3& translationVector, DAVA::Vector3& normal) const
{
    DAVA::Array<DAVA::uint16, 3> vertIndices;
    collision.geometry->GetTriangleIndices(collision.triangleIndex * 3, vertIndices.data());
    DAVA::Vector3 v[3];
    collision.geometry->GetCoord(vertIndices[0], v[0]);
    collision.geometry->GetCoord(vertIndices[1], v[1]);
    collision.geometry->GetCoord(vertIndices[2], v[2]);
    normal = (v[1] - v[0]).CrossProduct(v[2] - v[0]);
    translationVector = DAVA::Vector3(0.0f, 0.0f, -collision.t);
    DAVA::Matrix4* hitWorldTransform = collision.renderObject->GetWorldTransformPtr();
    DAVA::Quaternion rotationQuaternion;
    DAVA::Vector3 pos, scale;
    hitWorldTransform->Decomposition(pos, scale, rotationQuaternion);
    normal = MultiplyVectorMat3x3(normal, rotationQuaternion.GetMatrix());
}

void ObjectPlacementSystem::PrepareForRemove()
{
}
