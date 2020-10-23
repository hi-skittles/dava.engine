#ifndef __DAVAENGINE_TESTCLASSFACTORY_H__
#define __DAVAENGINE_TESTCLASSFACTORY_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
namespace UnitTests
{
class TestClass;

class TestClassFactoryBase
{
public:
    virtual ~TestClassFactoryBase();
    virtual TestClass* CreateTestClass() = 0;

protected:
    TestClassFactoryBase() = default;
};

template <typename T>
class TestClassFactoryImpl : public TestClassFactoryBase
{
public:
    TestClass* CreateTestClass() override
    {
        return new T;
    }
};

} // namespace UnitTests
} // namespace DAVA

#endif // __DAVAENGINE_TESTCLASSFACTORY_H__
