#ifndef __CLIPTEST_TEST_H__
#define __CLIPTEST_TEST_H__

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;
class ClipTest : public BaseScreen
{
public:
    ClipTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void MoveDown(BaseObject* obj, void* data, void* callerData);
    void MoveUp(BaseObject* obj, void* data, void* callerData);
    void MoveRight(BaseObject* obj, void* data, void* callerData);
    void MoveLeft(BaseObject* obj, void* data, void* callerData);
    void StartPos(BaseObject* obj, void* data, void* callerData);
    void DebugDrawPressed(BaseObject* obj, void* data, void* callerData);
    void ClipPressed(BaseObject* obj, void* data, void* callerData);

    bool enableClip = false;
    bool enableDebugDraw = false;
    Rect defaultRect;

    UIControl* fullSizeWgt;
    UIControl* parent1;
    UIControl* child1;
    UIControl* parent2;
    UIControl* child2;

    UIButton* clip;
    UIButton* debugDraw;
    UIButton* startPos;
    UIButton* moveLeft;
    UIButton* moveRight;
    UIButton* moveUp;
    UIButton* moveDown;
};

#endif //__CLIPTEST_TEST_H__
