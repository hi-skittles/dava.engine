#include "Base/ScopedPtr.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "FileSystem/File.h"
#include "Scripting/LuaScript.h"
#include "Scripting/LuaException.h"
#include "Scripting/Private/LuaBridge.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)
#include "MemoryManager/MemoryProfiler.h"

void* lua_profiler_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    if (0 == nsize)
    {
        DAVA::MemoryManager::Instance()->Deallocate(ptr);
        return nullptr;
    }

    void* newPtr = DAVA::MemoryManager::Instance()->Allocate(nsize, DAVA::ALLOC_POOL_LUA);
    if (osize != 0 && newPtr != nullptr)
    {
        size_t n = std::min(osize, nsize);
        Memcpy(newPtr, ptr, n);
        DAVA::MemoryManager::Instance()->Deallocate(ptr);
    }
    return newPtr;
}
#endif

DAVA::int32 panichandler(lua_State* L)
{
    std::ostringstream os;
    DAVA::LuaBridge::DumpStack(L, os);
    DAVA::LuaBridge::DumpCallstack(L, os);
    DAVA::Logger::Debug("LUA PANIC: unhandled error during Lua call:\n%s\n%s", lua_tostring(L, -1), os.str().c_str());
    return 0;
}

DAVA::int32 errorhandler(lua_State* L)
{
    std::ostringstream os;
    DAVA::LuaBridge::DumpStack(L, os);
    DAVA::LuaBridge::DumpCallstack(L, os);
    DAVA::Logger::Debug("Lua error:\n%s\n%s", lua_tostring(L, -1), os.str().c_str());
    return 1;
}

namespace DAVA
{
struct ScriptState
{
    lua_State* lua = nullptr;
};

LuaScript::LuaScript()
    : LuaScript(true)
{
}

LuaScript::LuaScript(bool initDefaultLibs)
    : errorHandlerRef(LUA_REFNIL)
{
    state = new ScriptState;
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    state->lua = lua_newstate(&lua_profiler_allocator, nullptr);
#else
    state->lua = luaL_newstate();
#endif

    RegisterErrorHandlers();

    if (initDefaultLibs)
    {
        luaL_openlibs(state->lua); // Load standard libs

        // Register in lua::package library our modules loader
        LuaBridge::RegisterModulesLoader(state->lua);
    }

    LuaBridge::RegisterDava(state->lua);
    LuaBridge::RegisterAny(state->lua);
    LuaBridge::RegisterAnyFn(state->lua);
    LuaBridge::RegisterReflection(state->lua);
}

LuaScript::LuaScript(LuaScript&& obj)
{
    std::swap(state, obj.state);
}

LuaScript::~LuaScript()
{
    if (state)
    {
        lua_unref(state->lua, errorHandlerRef);
        lua_close(state->lua);
        delete state;
    }
}

int32 LuaScript::ExecString(const String& script)
{
    int32 res = luaL_loadstring(state->lua, script.c_str()); // stack +1: script chunk
    if (res != 0)
    {
        DAVA_THROW(LuaException, res, LuaBridge::PopString(state->lua)); // stack -1
    }

    int32 base = lua_gettop(state->lua); // store current stack size
    DVASSERT(base >= 1, "Lua stack corrupted!");

    int32 errfunc = PushErrorHandler(base); // stack +1: insert error handler function before function
    res = lua_pcall(state->lua, 0, LUA_MULTRET, errfunc); // stack -1: run function/chunk on stack top and pop it
    int32 top = lua_gettop(state->lua); // store current stack size

    if (errfunc)
    {
        DVASSERT(top >= base, "Lua stack corrupted!");
        lua_remove(state->lua, base); // stack -1: remove error hander function
    }

    if (res != 0)
    {
        DAVA_THROW(LuaException, res, LuaBridge::PopString(state->lua)); // stack -1
    }

    return top - base; // calculate number of function results
}

int32 LuaScript::ExecStringSafe(const String& script)
{
    try
    {
        return ExecString(script);
    }
    catch (const LuaException& e)
    {
        Logger::Warning("LuaException: %s", e.what());
        return -1;
    }
}

int32 LuaScript::ExecScript(const FilePath& scriptPath)
{
    ScopedPtr<File> scriptFile(File::Create(scriptPath, File::OPEN | File::READ));
    if (!scriptFile)
    {
        DAVA_THROW(LuaException, LUA_ERRFILE, Format("Can't open file %s", scriptPath.GetStringValue().c_str()).c_str());
    }

    Vector<char8> buffer(static_cast<size_t>(scriptFile->GetSize()));
    int32 readed = scriptFile->Read(buffer.data(), static_cast<uint32>(buffer.size()));
    if (readed != buffer.size())
    {
        DAVA_THROW(LuaException, LUA_ERRFILE, Format("Error while reading file %s", scriptPath.GetStringValue().c_str()).c_str());
    }

    int32 res = luaL_loadbuffer(state->lua, buffer.data(), buffer.size(), scriptPath.GetStringValue().c_str());
    if (res != 0)
    {
        DAVA_THROW(LuaException, res, LuaBridge::PopString(state->lua)); // stack -1
    }

    int32 base = lua_gettop(state->lua); // store current stack size
    DVASSERT(base >= 1, "Lua stack corrupted!");

    int32 errfunc = PushErrorHandler(base); // stack +1: insert error handler function before function
    res = lua_pcall(state->lua, 0, LUA_MULTRET, errfunc); // stack -1: run function/chunk on stack top and pop it
    int32 top = lua_gettop(state->lua); // store current stack size

    if (errfunc)
    {
        DVASSERT(top >= base, "Lua stack corrupted!");
        lua_remove(state->lua, base); // stack -1: remove error hander function
    }

    if (res != 0)
    {
        DAVA_THROW(LuaException, res, LuaBridge::PopString(state->lua)); // stack -1
    }

    return top - base; // calculate number of function results
}

int32 LuaScript::ExecScriptSafe(const FilePath& scriptPath)
{
    try
    {
        return ExecScript(scriptPath);
    }
    catch (const LuaException& e)
    {
        Logger::Warning("LuaException: %s", e.what());
        return -1;
    }
}

int32 LuaScript::ExecFunction(const String& fName, const Vector<Any>& args)
{
    BeginCallFunction(fName);
    for (const Any& arg : args)
    {
        PushArg(arg);
    }
    return EndCallFunction(static_cast<int32>(args.size()));
}

int32 LuaScript::ExecFunctionSafe(const String& fName, const Vector<Any>& args)
{
    try
    {
        return ExecFunction(fName, args);
    }
    catch (const LuaException& e)
    {
        Logger::Warning(Format("LuaException: %s", e.what()).c_str());
        return -1;
    }
}

Vector<Any> LuaScript::ExecFunctionWithResult(String fName, const Vector<Any>& args, const Vector<const Type*>& returnTypes)
{
    BeginCallFunction(fName);
    for (const Any& arg : args)
    {
        PushArg(arg);
    }
    int32 count = EndCallFunction(static_cast<int32>(args.size()));
    Vector<Any> results;
    if (count != static_cast<int32>(returnTypes.size()))
    {
        DAVA_THROW(LuaException, -1, "Return values count not equals count of specified return types");
    }
    if (count > 0)
    {
        for (int32 i = 0; i < count; ++i)
        {
            results.push_back(GetResult(i + 1, returnTypes[i]));
        }
        Pop(count);
    }
    return results;
}

bool LuaScript::ExecFunctionWithResultSafe(String fName, const Vector<Any>& args, const Vector<const Type*>& returnTypes, Vector<Any>& returnValues)
{
    try
    {
        returnValues = ExecFunctionWithResult(fName, args, returnTypes);
        return true;
    }
    catch (const LuaException& e)
    {
        Logger::Warning(Format("LuaException: %s", e.what()).c_str());
        return false;
    }
}

Any LuaScript::GetResult(int32 index, const Type* preferredType /*= nullptr*/) const
{
    return LuaBridge::LuaToAny(state->lua, index, preferredType);
}

bool LuaScript::GetResultSafe(int32 index, Any& any, const Type* preferredType /*= nullptr*/) const
{
    try
    {
        any = LuaBridge::LuaToAny(state->lua, index, preferredType);
        return true;
    }
    catch (const LuaException& e)
    {
        Logger::Warning("LuaException: %s", e.what());
        return false;
    }
}

void LuaScript::Pop(int32 n)
{
    n = std::max(0, n);
    int32 size = lua_gettop(state->lua);
    n = std::min(n, size);
    lua_pop(state->lua, n);
}

void LuaScript::SetGlobalVariable(const String& vName, const Any& value)
{
    LuaBridge::AnyToLua(state->lua, value); // stack +1
    lua_setglobal(state->lua, vName.c_str()); // stack -1
}

bool LuaScript::HasGlobalVariable(const String& vName)
{
    lua_getglobal(state->lua, vName.c_str());
    bool found = !lua_isnone(state->lua, lua_gettop(state->lua));
    lua_pop(state->lua, 1);
    return found;
}

bool LuaScript::HasGlobalFunction(const String& fName)
{
    lua_getglobal(state->lua, fName.c_str());
    bool found = lua_isfunction(state->lua, lua_gettop(state->lua));
    lua_pop(state->lua, 1);
    return found;
}

void LuaScript::DumpStack(std::ostream& os) const
{
    LuaBridge::DumpStack(state->lua, os);
}

void LuaScript::DumpStackToLog(Logger::eLogLevel level) const
{
    std::ostringstream os;
    DumpStack(os);
    Logger* logger = GetEngineContext()->logger;
    if (logger)
    {
        logger->Log(level, os.str().c_str());
    }
}

void LuaScript::BeginCallFunction(const String& fName)
{
    lua_getglobal(state->lua, fName.c_str()); // stack +1: main() function
}

void LuaScript::PushArg(const Any& any)
{
    LuaBridge::AnyToLua(state->lua, any); // stack +1: function arg
}

int32 LuaScript::EndCallFunction(int32 nargs)
{
    int32 base = lua_gettop(state->lua) - nargs; // store function stack index
    DVASSERT(base >= 1, "Lua stack corrupted!");

    int32 errfunc = PushErrorHandler(base); // stack +1: insert error handler function before function
    int32 res = lua_pcall(state->lua, nargs, LUA_MULTRET, errfunc); // stack -(nargs+1), +nresults: return value or error message
    int32 top = lua_gettop(state->lua); // store current stack size (must contains error handler if it has setted)

    if (errfunc)
    {
        DVASSERT(top >= base, "Lua stack corrupted!");
        lua_remove(state->lua, base); // stack -1: remove error hander function
    }

    if (res != 0)
    {
        DAVA_THROW(LuaException, res, LuaBridge::PopString(state->lua)); // stack -1
    }

    return top - base; // calculate number of function results
}

void LuaScript::RegisterErrorHandlers()
{
    lua_atpanic(state->lua, &panichandler);
    lua_pushcfunction(state->lua, &errorhandler); // stack +1: put error handler to top of the stack
    errorHandlerRef = lua_ref(state->lua, 1); // stack -1: store function on top of the stack in registry table
}

int32 LuaScript::PushErrorHandler(int32 index)
{
    if (errorHandlerRef != LUA_NOREF && errorHandlerRef != LUA_REFNIL)
    {
        lua_getref(state->lua, errorHandlerRef); // stack +1: put error handler function to top of the stack
        lua_insert(state->lua, index); // move it to new position in the stack
        return 1;
    }
    return 0;
}
}
