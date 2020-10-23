#ifndef __DAVAENGINE_ICONTROLLER_H__
#define __DAVAENGINE_ICONTROLLER_H__

#include <Base/BaseTypes.h>
#include <Functional/Function.h>

namespace DAVA
{
namespace Net
{
struct IController
{
    // There should be a virtual destructor defined as objects may be deleted through this interface
    virtual ~IController();

    enum Status
    {
        NOT_STARTED,
        STARTED,
        START_FAILED
    };
    virtual Status GetStatus() const = 0;

    virtual void Start() = 0;
    virtual void Stop(Function<void(IController*)> callback) = 0;
    virtual void Restart() = 0;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_ICONTROLLER_H__
