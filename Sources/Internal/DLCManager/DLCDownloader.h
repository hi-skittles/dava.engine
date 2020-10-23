#pragma once

#include <atomic>
#include <iosfwd>

#include "Base/BaseTypes.h"
#include "Base/String.h"

namespace DAVA
{
class ProfilerCPU;
/**
	This class is for downloading with HTTP protocol. You can perform the
	following tasks:
	1. download file with one or more simultaneous curl easy handlers
	2. get remote file size
	3. resume previous download
	Also you can download into file or your custom buffer implementing
	```DLCDownloader::IWriter``` interface.
	Typical usage:
	```
	DLCDownloader* downloader = DLCDownloader::Create();

	// you can customize how many handles will be used and what buffer size
	// to use per handle
	Hints hints;
	hints.numOfMaxEasyHandles = 1; // we download small file one handle enough
	hints.chunkMemBuffSize = 16 * 1024; // 16 K for small file
	downloader->SetHints(hints);

	DLCDownloader::Task* task = downloader->StartTask(
	    "http://someserver/index.html,
	    "~doc:/file_from_server.file" ,
		DLCDownloader::TaskType::FULL);

	downloader->WaitTask(task);

	const TaskStatus& status = downloader->GetTaskStatus(task);

	if (!status.error.errorHappened)
	{
		// Use downloaded file ...
	}

	downloader->RemoveTask(task);

	DLCDownloader::Destroy(downloader);
	```
*/
class DLCDownloader
{
public:
    virtual ~DLCDownloader();
    /**
     You can customize some internal parameters before start requesting
     tasks.
     */
    struct Hints
    {
        Hints()
        {
        } // fix for clang++
        int32 numOfMaxEasyHandles = 8; //!< How many curl easy handles will be used
        int32 chunkMemBuffSize = 512 * 1024; //!< Max buffer size per one download operation per curl easy handler
        int32 timeout = 30; //!< Timeout in seconds for curl easy handlers to wait on connect, dns request etc.
        ProfilerCPU* profiler = nullptr; //!< performance checking
    };
    /** Create new instance of DLCDownloader. You can customize it with
	    ```DLCDownloader::SetHints(const Hints)``` right after creation. */
    static DLCDownloader* Create(const Hints& hints = Hints());
    /** Destroy downloader instance */
    static void Destroy(DLCDownloader* downloader);

    /**
		Task can be in one of next states.
	*/
    enum class TaskState : int32
    {
        JustAdded, //!< Just created and added to download queue
        Downloading, //!< One or more curl handles is working.
        Finished //!< All done, now you can remove task with ```DLCDownloader::RemoveTask(Task*)```
    };

    enum class TaskType : int32
    {
        FULL, //!< Truncate file if exist and download again
        RESUME, //!< Resume downloading from current output file(buffer) size
        SIZE //!< Just return size of remote file, see ```TaskStatus::sizeTotal```
    };

    /**
		If you need to output downloaded stream into socket or memory buffer
		You can implement ```IWriter``` interface and pass it to ```StartTask```
	*/
    struct IWriter
    {
        virtual ~IWriter() = default;
        /** Save next buffer bytes into memory or file, on error result != size */
        virtual uint64 Save(const void* ptr, uint64 size) = 0;
        /** Return current size of saved byte stream, return ```std::numeric_limits<uint64>::max()``` value on error */
        virtual uint64 GetSeekPos() = 0;
        /** Truncate file(or buffer) to zero length, return false on error */
        virtual bool Truncate() = 0;
        /** Close internal resource (file handle, socket, free memory)
		    return true on success
		*/
        virtual bool Close() = 0;
        /** Check internal state */
        virtual bool IsClosed() const = 0;
    };

    struct Range
    {
        Range();
        Range(int64 offset_, int64 size_)
            : offset(offset_)
            , size(size_)
        {
        }
        int64 offset = -1;
        int64 size = -1;
    };

    static const Range EmptyRange;

    /**
		Information for task during start
	*/
    struct TaskInfo
    {
        String srcUrl; //!< URL to download from
        String dstPath; //!< Path to file or empty string if loading into custom IWriter
        TaskType type = TaskType::RESUME; //!< Type of download action
        int32 timeoutSec = 30; //!< Timeout in seconds
        int64 rangeOffset = -1; //!< Index of first byte to download from or -1 [0 - first byte index]
        int64 rangeSize = -1; //!< Size of downloaded range in bytes
    };
    /**
		Four types of error can happen during download process. All grouped
		in one structure ```TaskError```.
		1. problem with curl easy interface
		2. problem with curl multi interface
		3. problem with file operation
		4. bad http request code
		If any one error happened boolean flag ```errorHappened``` is set to true.
	*/
    struct TaskError
    {
        int32 curlErr = 0; //!< CURLE_OK == 0 see https://curl.haxx.se/libcurl/c/libcurl-errors.html
        int32 curlMErr = 0; //!< CURLM_OK == 0 see https://curl.haxx.se/libcurl/c/libcurl-errors.html
        int32 fileErrno = 0; //!< Errno value after bad (open|read|write|close|truncate) operation to file
        int32 httpCode = 0; //!< Last received HTTP response code
        int32 fileLine = 0; //!< Source code first known line with error
        //!< See http://en.cppreference.com/w/cpp/error/errno_macros
        const char* errStr = ""; //!< See https://curl.haxx.se/libcurl/c/curl_multi_strerror.html
        //!< And https://curl.haxx.se/libcurl/c/curl_easy_strerror.html
        bool errorHappened = false; //!< Flag set to true if any error
    };
    /**
		Current status of any task.
	*/
    struct TaskStatus
    {
        std::atomic<TaskState> state{ TaskState::JustAdded }; //!< Current state
        TaskError error; //!< Full error info
        uint64 sizeTotal = 0; //!< Size of remote file or range size to download
        uint64 sizeDownloaded = 0; //!< Size written into IWritable(file)

        TaskStatus();
        TaskStatus(const TaskStatus& other);
        TaskStatus& operator=(const TaskStatus& other);
    };

    struct ITask
    {
        virtual ~ITask() = default;
    };

    /** Start http request to find out content size. */
    virtual ITask* StartGetContentSize(const String& srcUrl) = 0;
    /** Start downloading to dstPath file */
    virtual ITask* StartTask(const String& srcUrl, const String& dstPath, Range range = EmptyRange) = 0;
    /** Start downloading to custom writer
	    You can reuse customWriter after finish Task
	*/
    virtual ITask* StartTask(const String& srcUrl, std::shared_ptr<IWriter> customWriter, Range range = EmptyRange) = 0;
    /** Resume downloading to file starting from current file size */
    virtual ITask* ResumeTask(const String& srcUrl, const String& dstPath, Range range = EmptyRange) = 0;
    /** Resume downloading to custom writer starting from current position
	    You can reuse customWriter after finish Task
	*/
    virtual ITask* ResumeTask(const String& srcUrl, std::shared_ptr<IWriter> customWriter, Range range = EmptyRange) = 0;

    /**  Clear task data and free resources */
    virtual void RemoveTask(ITask* task) = 0;

    /** Wait for task status == finished */
    virtual void WaitTask(ITask* task) = 0;

    virtual const TaskInfo& GetTaskInfo(ITask* task) = 0;
    virtual const TaskStatus& GetTaskStatus(ITask* task) = 0;

    /** You can customize internal curl behavior. Do it just after new
	```DLCDownloader``` created and before any task started.*/
    virtual void SetHints(const Hints& h) = 0;
};

std::ostream& operator<<(std::ostream&, const DLCDownloader::TaskError&);
std::ostream& operator<<(std::ostream&, const DLCDownloader::TaskStatus&);
}
