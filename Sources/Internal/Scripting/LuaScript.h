#pragma once

#include "Base/BaseTypes.h"
#include "Base/Any.h"
#include "Scripting/LuaException.h"
#include "Logger/Logger.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
struct ScriptState;

/**
Class for Lua script.
*/
class LuaScript final
{
public:
    /**
    Create script with default Lua libraries.
    */
    LuaScript();

    /**
    Create script with or without default Lua libraries.
    */
    LuaScript(bool initDefaultLibs);

    /**
    Move script state to another object.
    */
    LuaScript(LuaScript&&);

    /**
    Deleted copy constructor. Lua state is non-copyable.
    */
    LuaScript(const LuaScript&) = delete;

    /**
    Destroy the object and free Lua state.
    */
    ~LuaScript();

    /**
    Deleted assign operator. Lua state is non-copyable.
    */
    LuaScript& operator=(const LuaScript&) = delete;

    /**
    Load script from string, run it and return number of results in the stack.
    Throw LuaException on error.
    */
    int32 ExecString(const String& script);

    /**
    Load script from string, run it and return number of results in the stack.
    Return -1 on error.
    */
    int32 ExecStringSafe(const String& script);

    /**
    Load script from file, run it and return number of results in the stack.
    Throw LuaException on error.
    */
    int32 ExecScript(const FilePath& scriptPath);

    /**
    Load script from file, run it and return number of results in the stack.
    Return -1 on error.
    */
    int32 ExecScriptSafe(const FilePath& scriptPath);

    /**
    Run `fName(...)` function with arguments and return number of results in
    the stack.
    Throw LuaException on error.
    */
    template <typename... T>
    int32 ExecFunction(const String& fName, T&&... args);

    /**
    Run `fName(...)` function with arguments and return number of results in
    the stack.
    Throw LuaException on error.
    */
    int32 ExecFunction(const String& fName, const Vector<Any>& args);

    /**
    Run `fName(...)` function with arguments and return number of results in
    the stack.
    Return -1 on error.
    */
    template <typename... T>
    int32 ExecFunctionSafe(const String& fName, T&&... args);

    /**
    Run `fName(...)` function with arguments and return number of results in
    the stack.
    Return -1 on error.
    */
    int32 ExecFunctionSafe(const String& fName, const Vector<Any>& args);

    /**
    Run `fName(...)` function with arguments and specified results types and 
    return vector of results.
    Throw LuaException on error.
    */
    Vector<Any> ExecFunctionWithResult(String fName, const Vector<Any>& args, const Vector<const Type*>& returnTypes);

    /**
    Run `fName(...)` function with arguments, specified results types and write 
    results to specified vector.
    Return false on error.
    */
    bool ExecFunctionWithResultSafe(String fName, const Vector<Any>& args, const Vector<const Type*>& returnTypes, Vector<Any>& returnValues);

    /**
    Return value at specified index on the stack as Any with specified type.
    Acceptable index is in range [1, stack size] or [-stack size, -1].
    After process all results you **must** `Pop` them.
    Throw LuaException or error.
    */
    Any GetResult(int32 index, const Type* preferredType = nullptr) const;

    /**
    Return value at specified index on the stack as Any with specified type.
    Acceptable index is in range [1, stack size] or [-stack size, -1].
    After process all results you **must** `Pop` them.
    Throw LuaException or error.
    */
    template <typename T>
    Any GetResult(int32 index) const;

    /**
    Return value at specified index on the stack as Any with specified type.
    Acceptable index is in range [1, stack size] or [-stack size, -1].
    After process all results you **must** `Pop` them.
    Return false on error.
    */
    bool GetResultSafe(int32 index, Any& any, const Type* preferredType = nullptr) const;

    /**
    Return value at specified index on the stack as Any with specified type.
    Acceptable index is in range [1, stack size] or [-stack size, -1].
    After process all results you **must** `Pop` them.
    Return false on error.
    */
    template <typename T>
    bool GetResultSafe(int32 index, Any& any) const;

    /**
    Pop n-results from the stack or clear stack if specified `n` bigger than
    size of the stack.
    */
    void Pop(int32 n);

    /**
    Set variable to global table with name `vName`
    */
    void SetGlobalVariable(const String& vName, const Any& value);

    /**
    Check variable in global table with name `vName`
    */
    bool HasGlobalVariable(const String& vName);

    /**
    Check function in global table with name `vName`
    */
    bool HasGlobalFunction(const String& vName);

    /**
    Dump current content of the stack to `ostream`.
    */
    void DumpStack(std::ostream& os) const;

    /**
    Dump current content of the stack to log with specified log level.
    */
    void DumpStackToLog(Logger::eLogLevel level) const;

private:
    ScriptState* state = nullptr; //!< Internal script state

    /**
    Find function with name `fName` and put at top of the stack.
    */
    void BeginCallFunction(const String& fName);

    /**
    Put any value at top of the stack.
    */
    void PushArg(const Any& any);

    /**
    Call Lua function with `nargs` arguments on top of stack, pop they
    and return number of function results in stack.
    Throw LuaException on error.
    */
    int32 EndCallFunction(int32 nargs);

    /**
    Register error handling functions
    */
    void RegisterErrorHandlers();

    /**
    Push error handler function to Lua stack at specified index.
    Return 1 if pushed or 0 if not. This return value can be used as `errfunc`
    argument in `lua_pcall`.
    */
    int32 PushErrorHandler(int32 index);

    int32 errorHandlerRef; //<! Unique index of error handler function in global Lua namespace
};

template <typename... T>
inline int32 LuaScript::ExecFunction(const String& fName, T&&... args)
{
    BeginCallFunction(fName);
    const int32 size = sizeof...(args);
    bool vargs[] = { true, (PushArg(Any(std::forward<T>(args))), true)... };
    return EndCallFunction(size);
}

template <typename... T>
inline int32 LuaScript::ExecFunctionSafe(const String& fName, T&&... args)
{
    try
    {
        return ExecFunction(fName, std::forward<T>(args)...);
    }
    catch (const LuaException& e)
    {
        DAVA::Logger::Warning(Format("LuaException: %s", e.what()).c_str());
        return -1;
    }
}

template <typename T>
inline Any LuaScript::GetResult(int32 index) const
{
    return GetResult(index, Type::Instance<T>());
}

template <typename T>
inline bool LuaScript::GetResultSafe(int32 index, Any& any) const
{
    return GetResultSafe(index, any, Type::Instance<T>());
}
}
