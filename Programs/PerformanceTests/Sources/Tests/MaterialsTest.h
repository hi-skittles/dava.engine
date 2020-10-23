#ifndef __MATERIALS_TEST_H__
#define __MATERIALS_TEST_H__

#include "BaseTest.h"

class MaterialsTest : public BaseTest
{
public:
    MaterialsTest(const TestParams& testParams);

    void BeginFrame() override;

    bool IsFinished() const override;

    static const String TEST_NAME;

protected:
    void LoadResources() override;
    void UnloadResources() override;

    void PerformTestLogic(float32 timeElapsed) override{};
    void PrintStatistic(const Vector<BaseTest::FrameInfo>& frames) override;

    const String& GetSceneName() const override;

private:
    Entity* CreateSpeedTreeEntity(Entity* entity);
    Entity* CreateSkinnedEntity(Entity* entity);
    Entity* CreateEntityForLightmapMaterial(Entity* entity);

    void ReplacePlanes(const Vector<Entity*>& planes);

    static const String SPHERICAL_LIT_MATERIAL;
    static const String SKINNED_MATERIAL;
    static const String LIGHTMAP_MATERIAL;

    static const FastName LIGHT_ENTITY;
    static const FastName CAMERA_ENTITY;
    static const FastName PLANE_ENTITY;
    static const FastName MATERIALS_ENTITY;

    static const uint32 FRAMES_PER_MATERIAL_TEST;

    int32 currentTestStartFrame;
    int64 currentTestStartTimeMs;
    uint32 currentMaterialIndex;

    Vector<Entity*> planes;
    Vector<Entity*> spoPlanes;
    Vector<Entity*> skinnedPlanes;
    Vector<Entity*> lightmapMaterialPlanes;

    Vector<NMaterial*> materials;
    Vector<float32> materialTestsElapsedTime;
};

#endif
