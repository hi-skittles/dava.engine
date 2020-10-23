#ifndef __SPEED_LOAD_IMAGES_TEST__
#define __SPEED_LOAD_IMAGES_TEST__

#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class SpeedLoadImagesTest : public BaseScreen
{
protected:
    ~SpeedLoadImagesTest() override;

public:
    SpeedLoadImagesTest(TestBed& app);

    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnTestPng(BaseObject* obj, void* data, void* callerData);
    void OnTestJpg(BaseObject* obj, void* data, void* callerData);
    void OnTestTga(BaseObject* obj, void* data, void* callerData);
    void OnTestWebP(BaseObject* obj, void* data, void* callerData);
    void OnTestPvr(BaseObject* obj, void* data, void* callerData);

    void TestAndDisplayFormat(String extension, const Vector<String>& qualities);
    void CreatePaths(String extension, const Vector<String>& qualities, Vector<FilePath>& outPaths);
    uint64 GetLoadTime(const FilePath& path);

private:
    UIStaticText* resultText;
};

#endif /* defined(__SPEED_LOAD_IMAGES_TEST__) */
