#include "FileSystem/Private/CheckIOError.h"
#include "Debug/DVAssert.h"
#include <cerrno>

namespace DAVA
{
namespace DebugFS
{
static IOErrorTypes ioErrors;
void GenerateIOErrorOnNextOperation(IOErrorTypes types)
{
    bool anyError = types.closeFailed ||
    types.openOrCreateFailed ||
    types.readFailed ||
    types.seekFailed ||
    types.truncateFailed ||
    types.writeFailed ||
    types.moveFailed;

    if (types.ioErrorCode != 0)
    {
        DVASSERT(anyError);
    }
    else
    {
        DVASSERT(anyError == false);
    }
    ioErrors = types;
}
bool GenErrorOnOpenOrCreateFailed()
{
    if (ioErrors.openOrCreateFailed)
    {
        errno = ioErrors.ioErrorCode;
        return true;
    }
    return false;
}
bool GenErrorOnWriteFailed()
{
    if (ioErrors.writeFailed)
    {
        errno = ioErrors.ioErrorCode;
        return true;
    }
    return false;
}
bool GenErrorOnReadFailed()
{
    if (ioErrors.readFailed)
    {
        errno = ioErrors.ioErrorCode;
        return true;
    }
    return false;
}
bool GenErrorOnSeekFailed()
{
    if (ioErrors.seekFailed)
    {
        errno = ioErrors.ioErrorCode;
        return true;
    }
    return false;
}
bool GenErrorOnCloseFailed()
{
    if (ioErrors.closeFailed)
    {
        errno = ioErrors.ioErrorCode;
        return true;
    }
    return false;
}
bool GenErrorOnTruncateFailed()
{
    if (ioErrors.truncateFailed)
    {
        errno = ioErrors.ioErrorCode;
        return true;
    }
    return false;
}
bool GenErrorOnMoveFailed()
{
    if (ioErrors.moveFailed)
    {
        errno = ioErrors.ioErrorCode;
        return true;
    }
    return false;
}
}
} // end namespace DAVA
