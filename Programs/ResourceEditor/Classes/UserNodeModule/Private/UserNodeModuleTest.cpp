#include "Classes/UserNodeModule/UserNodeModule.h"

#include <TArc/Testing/TArcUnitTests.h>

#include <Base/ScopedPtr.h>
#include <FileSystem/FilePath.h>
#include <Render/Highlevel/RenderObject.h>
#include <Scene3D/Scene.h>
#include <Scene3D/SceneFileV2.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/RenderComponent.h>

DAVA_TARC_TESTCLASS(UserModuleTest)
{
    DAVA_TEST (TankObjectTest)
    {
        using namespace DAVA;

        FilePath scenePath = UserNodeModule::GetBotSpawnPath();

        ScopedPtr<Scene> scene(new Scene());
        SceneFileV2::eError result = scene->LoadScene(scenePath);
        if (result == SceneFileV2::ERROR_NO_ERROR)
        {
            Vector<Entity*> entities;
            scene->GetChildEntitiesWithComponent(entities, DAVA::Type::Instance<DAVA::RenderComponent>());
            if (entities.size() == 1)
            {
                TEST_VERIFY(GetRenderObject(entities[0]) != nullptr);
            }
            else
            {
                TEST_VERIFY(false);
            }
        }
        else
        {
            TEST_VERIFY(false);
        }
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("UserModule.cpp")
    END_FILES_COVERED_BY_TESTS();
}
;
