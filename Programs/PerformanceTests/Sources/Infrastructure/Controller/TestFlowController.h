#ifndef __TEST_FLOW_CONTROLLER_H__
#define __TEST_FLOW_CONTROLLER_H__

#include "Tests/BaseTest.h"

class TestFlowController
{
public:
    virtual void Init(const Vector<BaseTest*>& registeredTests);
    virtual void Finish(){};

    virtual void Update(float32 delta){};

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    virtual ~TestFlowController(){};

protected:
    Vector<BaseTest*> testChain;
};

#endif