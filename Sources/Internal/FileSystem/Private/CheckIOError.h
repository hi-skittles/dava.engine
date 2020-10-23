#pragma once

namespace DAVA
{
/**
	Example: We want to generate IO error - no space on HDD

	```
	DAVA::DebugFS::IOErrorTypes ioErr;
	ioErr.ioErrorCode = ENOSPC;
	ioErr.openOrCreateFailed = true;
	ioErr.writeFailed = true;

	GenerateIOErrorOnNextOperation(ioErr);
	```

	Any call To File::Create or File::Write will generate IO error with code ENOSPC from <cerrno>
*/
namespace DebugFS
{
struct IOErrorTypes
{
    // most interesting error codes(http://en.cppreference.com/w/cpp/header/cerrno):
    // ENOSPC - No space left on device
    // ENOENT - No such file or directory
    // ENFILE - Too many files open in system
    int ioErrorCode = 0;
    bool openOrCreateFailed = false;
    bool writeFailed = false;
    bool readFailed = false;
    bool seekFailed = false;
    bool closeFailed = false;
    bool truncateFailed = false;
    bool moveFailed = false;
};
void GenerateIOErrorOnNextOperation(IOErrorTypes types);
bool GenErrorOnOpenOrCreateFailed();
bool GenErrorOnWriteFailed();
bool GenErrorOnReadFailed();
bool GenErrorOnSeekFailed();
bool GenErrorOnCloseFailed();
bool GenErrorOnTruncateFailed();
bool GenErrorOnMoveFailed();
}
} // end namespace DAVA
