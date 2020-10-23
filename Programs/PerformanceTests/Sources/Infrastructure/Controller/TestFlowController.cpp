#include "TestFlowController.h"

void TestFlowController::Init(const Vector<BaseTest*>& _testChain)
{
    DVASSERT(!_testChain.empty());
    testChain = _testChain;
}