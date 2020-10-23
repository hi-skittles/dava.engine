#pragma once

#include "Base/Result.h"
#include "ValidationProgress.h"

namespace DAVA
{
class Scene;
}

class ProjectManagerData;

class SceneValidation
{
public:
    SceneValidation(ProjectManagerData* data);
    /**
    For all parent models (i.e. models at which `scene` entities are referenced with "referenceToOwner" property)
    function checks whether local & world matrices are identity.
    The progress and result of validating are available through `progress` variable
    */
    void ValidateMatrices(DAVA::Scene* scene, ValidationProgress& progress);

    /**
    For all `scene` entities with same names function does following checks:
    a. "CollisionType", "CollisionTypeCrashed", "editor.ReferenceToOwner", "Health", "MaterialKind" corresponding properties should be equal.
    b. Sound components should be equal. This means that corresponding sound events should be equal
       Sound events are considered to be equal when
       1. event names are equal
       2. event min,max distance are equal
       3. event properties are equal
    c. Child effect entities should be equal. 
       Effects are considered to be equal when all corresponding effect entities have same names
    The progress and result of validating are available through `progress` variable
    */
    void ValidateSameNames(DAVA::Scene*, ValidationProgress& progress);

    /**
    For all `scene` entities that have "CollisionType" property specifed, "MaterialKind" or "FallType" properties are also should be specifed.
    Except for entites that have "CollisionType" = "Water".
    The progress and result of validating are available through `progress` variable
    */
    void ValidateCollisionProperties(DAVA::Scene* scene, ValidationProgress& progress);

    /**
    For all `scene` textures that have any converted textures, function checks whether converted textures are relevant, i.e. recompression is not needed
    The progress and result of validating are available through `progress` variable
    */
    void ValidateTexturesRelevance(DAVA::Scene* scene, ValidationProgress& progress);

    /**
    For all `scene` materials that have multi-quality material template, function checks whether quality group is specified.
    The progress and result of validating are available through `progress` variable
    */
    void ValidateMaterialsGroups(DAVA::Scene* scene, ValidationProgress& progress);

private:
    ProjectManagerData* projectManagerData = nullptr;
};
