#pragma once

#include "Base/BaseTypes.h"
#include "Base/Any.h"
#include "Base/Platform.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace DAVA
{
namespace LuaBridge
{
/**
Register in Lua new DV namespace with service functions.
*/
void RegisterDava(lua_State* L);

/**
Register in lua::package library our modules loader.
*/
void RegisterModulesLoader(lua_State* L);

/**
Register in Lua metatable for Any userdata type.
*/
void RegisterAny(lua_State* L);

/**
Register in Lua metatable for AnyFn userdata type.
*/
void RegisterAnyFn(lua_State* L);

/**
Register in Lua metatable for Reflection userdata type.
*/
void RegisterReflection(lua_State* L);

/**
Gets Lua variable from the stack with specified index and convert it to Any.
*/
Any LuaToAny(lua_State* L, int32 index, const Type* preferredType = nullptr);

/**
Put specified value as Lua variable to top of the stack.
*/
void AnyToLua(lua_State* L, const Any& value);

/**
Get string from top of stack and pop it.
*/
String PopString(lua_State* L);

/**
Dump Lua stack to output stream.
*/
void DumpStack(lua_State* L, std::ostream& os);
/**
Dump Lua callstack to output stream. Can be called from error handler only!
*/
void DumpCallstack(lua_State* L, std::ostream& os);
}
}
