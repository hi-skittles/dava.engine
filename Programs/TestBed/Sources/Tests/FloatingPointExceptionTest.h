#ifndef __FLOATINGPOINTEXCEPTIONTEST_TEST_H__
#define __FLOATINGPOINTEXCEPTIONTEST_TEST_H__

#include <DAVAEngine.h>
#include "Infrastructure/BaseScreen.h"

class TestBed;
class FloatingPointExceptionTest : public BaseScreen
{
public:
    FloatingPointExceptionTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
};

#endif //__FLOATINGPOINTEXCEPTIONTEST_TEST_H__
