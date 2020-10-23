// clang-format off
#include "Debug/Backtrace.h"
#include "Utils/StringFormat.h"

#if defined(__DAVAENGINE_WIN32__)
#   include "Concurrency/Atomic.h"
#   include "Concurrency/Mutex.h"
#   include "Concurrency/LockGuard.h"
#pragma warning(push)
#pragma warning (disable: 4091) // 'typedef ': ignored on left of '' when no variable is declared
#   include <dbghelp.h>
#pragma warning(pop)
#elif defined(__DAVAENGINE_WIN_UAP__)
#   include "Concurrency/Atomic.h"
#   include "Concurrency/Mutex.h"
#   include "Concurrency/LockGuard.h"
#   include "Utils/Utils.h"
#   include "Utils/UTF8Utils.h"

// Types and prototypes from dbghelp.h
#define MAX_SYM_NAME            2000

typedef struct _SYMBOL_INFO {
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;        // Type Index of symbol
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;          // Base Address of module comtaining this symbol
    ULONG       Flags;
    ULONG64     Value;            // Value of symbol, ValuePresent should be 1
    ULONG64     Address;          // Address of symbol including base address of module
    ULONG       Register;         // register holding value or pointer to value
    ULONG       Scope;            // scope of the symbol
    ULONG       Tag;              // pdb classification
    ULONG       NameLen;          // Actual length of name
    ULONG       MaxNameLen;
    CHAR        Name[1];          // Name of symbol
} SYMBOL_INFO, *PSYMBOL_INFO;

BOOL (__stdcall *SymInitialize_impl)(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess);
BOOL (__stdcall *SymFromAddr_impl)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol);
BOOL (__stdcall *SymGetSearchPath_impl)(HANDLE hProcess, PSTR SearchPath, DWORD SearchPathLength);

// Wrapper function to use same code for win32 and winuap
BOOL SymInitialize(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess)
{
    if (SymInitialize_impl != nullptr)
        return SymInitialize_impl(hProcess, UserSearchPath, fInvadeProcess);
    return FALSE;
}

BOOL SymFromAddr(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol)
{
    if (SymFromAddr_impl != nullptr)
        return SymFromAddr_impl(hProcess, Address, Displacement, Symbol);
    return FALSE;
}

BOOL SymGetSearchPath(HANDLE hProcess, PSTR SearchPath, DWORD SearchPathLength)
{
    if (SymGetSearchPath_impl != nullptr)
        return SymGetSearchPath_impl(hProcess, SearchPath, SearchPathLength);
    return FALSE;
}

#elif defined(__DAVAENGINE_APPLE__) || defined(__DAVAENGINE_LINUX__)
#   include <execinfo.h>
#   include <dlfcn.h>
#   include <cxxabi.h>
#elif defined(__DAVAENGINE_ANDROID__)
#   include <dlfcn.h>
#   include <cxxabi.h>
#   include <unwind.h>
#   include <android/log.h>
#endif

namespace DAVA
{
namespace Debug
{
namespace BacktraiceDetails
{
#if defined(__DAVAENGINE_WINDOWS__)
void InitSymbols()
{
    static Atomic<bool> symbolsInited(false);
    if (!symbolsInited)
    {
        // All DbgHelp functions are single threaded
        static Mutex initMutex;
        LockGuard<Mutex> lock(initMutex);
        if (!symbolsInited)
        {

#if defined(__DAVAENGINE_WIN_UAP__)
            // Step into land of black magic and fire-spitting dragons
            // as microsoft forbids direct using of dbghelp functions
            MEMORY_BASIC_INFORMATION bi;
            VirtualQuery(static_cast<void*>(&GetModuleFileNameA), &bi, sizeof(bi));
            HMODULE hkernel = reinterpret_cast<HMODULE>(bi.AllocationBase);

            HMODULE (WINAPI *LoadLibraryW)(LPCWSTR lpLibFileName);
            LoadLibraryW = reinterpret_cast<decltype(LoadLibraryW)>(GetProcAddress(hkernel, "LoadLibraryW"));

            if (LoadLibraryW != nullptr)
            {
                HMODULE hdbghelp = LoadLibraryW(L"dbghelp.dll");
                if (hdbghelp)
                {
                    SymInitialize_impl = reinterpret_cast<decltype(SymInitialize_impl)>(GetProcAddress(hdbghelp, "SymInitialize"));
                    SymFromAddr_impl = reinterpret_cast<decltype(SymFromAddr_impl)>(GetProcAddress(hdbghelp, "SymFromAddr"));
                    SymGetSearchPath_impl = reinterpret_cast<decltype(SymGetSearchPath_impl)>(GetProcAddress(hdbghelp, "SymGetSearchPath"));
                }
            }

            // Winuap application has read-only access to executable folder and read-write access to application folder
            // So to correctly show function names in callstack user must copy pdb file into one of the following locations:
            // - executable folder: you can copy pdb only when running app from IDE
            // - application folder: %LOCALAPPDATA%\Packages\[app-guid]\LocalState
            // If pdb file is placed somewhere else application will have no access to it
            // dbghelp functions are available only for desktop platfoms not mobile
            using Windows::Storage::ApplicationData;
            String path = UTF8Utils::EncodeToUTF8(ApplicationData::Current->LocalFolder->Path->Data());
            path += ";.";
            const char* symPath = path.c_str();
#else
            const char* symPath = nullptr;
#endif

            SymInitialize(GetCurrentProcess(), symPath, TRUE);
            // Do not regard return value of SymInitialize: if this call failed then next call will likely fail too
            symbolsInited = true;
        }
    }
}
#endif

#if defined(__DAVAENGINE_ANDROID__)

struct StackCrawlState
{
    size_t count;
    void** frames;
};

_Unwind_Reason_Code TraceFunction(struct _Unwind_Context* context, void* arg)
{
    StackCrawlState* state = static_cast<StackCrawlState*>(arg);
    if (state->count > 0)
    {
        uintptr_t pc = _Unwind_GetIP(context);
        if (pc != 0)
        {
            *state->frames = reinterpret_cast<void*>(pc);
            state->frames += 1;
            state->count -= 1;
            return _URC_NO_REASON;
        }
    }
    return _URC_END_OF_STACK;
}
#endif
} // namespace BacktraiceDetails

String DemangleFrameSymbol(const char8* symbol)
{
#if defined(__DAVAENGINE_WINDOWS__)
    // On Win32 SymFromAddr returns already undecorated name
    return String(symbol);
#elif defined(__DAVAENGINE_POSIX__)
    String result;
    char* demangled = abi::__cxa_demangle(symbol, nullptr, nullptr, nullptr);
    if (demangled != nullptr)
    {
        result = demangled;
        free(demangled);
    }
    return result;
#endif
}

String DemangleFrameSymbol(const String& symbol)
{
    return DemangleFrameSymbol(symbol.c_str());
}

String GetFrameSymbol(void* frame, bool demangle)
{
    String result;
#if defined(__DAVAENGINE_WINDOWS__)
    const size_t NAME_BUFFER_SIZE = MAX_SYM_NAME + sizeof(SYMBOL_INFO);
    char8 nameBuffer[NAME_BUFFER_SIZE];

    SYMBOL_INFO* symInfo = reinterpret_cast<SYMBOL_INFO*>(nameBuffer);
    symInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    symInfo->MaxNameLen = NAME_BUFFER_SIZE - sizeof(SYMBOL_INFO);

    {
        BacktraiceDetails::InitSymbols();

        // All DbgHelp functions are single threaded
        static Mutex mutex;
        LockGuard<Mutex> lock(mutex);
        HANDLE currentProcess = GetCurrentProcess();
        if (SymFromAddr(currentProcess, reinterpret_cast<DWORD64>(frame), nullptr, symInfo))
        {
            const DWORD maxNameSize = 1024;
            CHAR moduleFileName[maxNameSize];
            DWORD moduleNameLength = GetModuleFileNameA(reinterpret_cast<HMODULE>(symInfo->ModBase), moduleFileName, maxNameSize);
            if (moduleNameLength != 0)
            {
                const char* moduleName = strrchr(moduleFileName, '\\');
                result = moduleName != nullptr ? (moduleName + 1) : moduleFileName;
                result += "!";
            }
            result += symInfo->Name;
        }
    }

#elif defined(__DAVAENGINE_POSIX__)
    Dl_info dlinfo;
    if (dladdr(frame, &dlinfo) != 0 && dlinfo.dli_sname != nullptr)
    {
        // Include SO name
        if (dlinfo.dli_fname != nullptr)
        {
            const char* moduleName = strrchr(dlinfo.dli_fname, '/');
            result = moduleName != nullptr ? (moduleName + 1) : dlinfo.dli_fname;
            result += '!';
        }

        if (demangle)
        {
            String demSym = DemangleFrameSymbol(dlinfo.dli_sname);
            result += demSym.empty() ? dlinfo.dli_sname : demSym;
        }
    }
#endif
    return result;
}

DAVA_NOINLINE size_t GetBacktrace(void** frames, size_t depth)
{
    size_t sz = 0;

    if (depth > 0)
    {
#if defined(__DAVAENGINE_WINDOWS__)
        // CaptureStackBackTrace is supported either on Win32 and WinUAP
        sz = CaptureStackBackTrace(0, static_cast<DWORD>(depth), frames, nullptr);
#elif defined(__DAVAENGINE_APPLE__) || defined(__DAVAENGINE_LINUX__)
        sz = backtrace(frames, static_cast<int>(depth));
#elif defined(__DAVAENGINE_ANDROID__)
        BacktraiceDetails::StackCrawlState state;
        state.count = depth;
        state.frames = frames;
        _Unwind_Backtrace(&BacktraiceDetails::TraceFunction, &state);
        sz = depth - state.count;
#endif
    }

    return sz;
}

DAVA_NOINLINE Vector<void*> GetBacktrace(size_t depth)
{
    const size_t DEFAULT_SKIP_COUNT = 3;
    const size_t MAX_BACKTRACE_COUNT = 64;
    void* frames[MAX_BACKTRACE_COUNT];

    depth = std::min(depth, MAX_BACKTRACE_COUNT);

    size_t sz = GetBacktrace(frames, depth);
    size_t firstFrame = 0;

    // Skip irrelevant GetStackFrames and GetBacktrace functions by name as different compilers
    // can include or exclude GetStackFrames function depending on compiler wish
    // TODO: find the way to tell ios compiler not inline GetStackFrames function (DAVA_NOINLINE does not help)
    if (sz > 0)
    {
        for (; firstFrame < std::min(sz, DEFAULT_SKIP_COUNT); ++firstFrame)
        {
            String s = GetFrameSymbol(frames[firstFrame]);
            if (s.find("DAVA::Debug::GetBacktrace") == String::npos)
            {
                break;
            }
        }
    }

    sz = sz - firstFrame;

    Vector<void*> ret(sz);
    for (size_t i = 0; i < sz; ++i)
    {
        ret[i] = frames[firstFrame + i];
    }

    return ret;
}

String GetBacktraceString(void* const* frames, size_t framesSize)
{
    std::ostringstream result;

    for (size_t i = 0; i < framesSize; ++i)
    {
        void *frame = frames[i];
        result << Format("    #%2u: [%p] %s\n", static_cast<uint32>(i), frame, GetFrameSymbol(frame).c_str());
    }

    return result.str();
}

String GetBacktraceString(const Vector<void*>& backtrace)
{
    return GetBacktraceString(backtrace.data(), backtrace.size());
}

String GetBacktraceString(const Vector<void*>& backtrace, size_t framesSize)
{
    const size_t length = (framesSize == 0) ? backtrace.size() : std::min(backtrace.size(), framesSize);
    return GetBacktraceString(backtrace.data(), length);
}

String GetBacktraceString(size_t depth)
{
    return GetBacktraceString(GetBacktrace(depth));
}

} // namespace Debug
} // namespace DAVA
