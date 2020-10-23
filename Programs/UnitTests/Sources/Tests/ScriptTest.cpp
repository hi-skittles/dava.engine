#include "UnitTests/UnitTests.h"
#include "Base/BaseTypes.h"
#include "Math/Color.h"
#include "Math/MathDefines.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scripting/LuaScript.h"

struct ReflClass : public DAVA::ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(ReflClass);

    ReflClass()
        : colorVal(DAVA::Color::White)
    {
    }

    bool returnTrue()
    {
        return true;
    }

    DAVA::int32 invert(DAVA::int32 value)
    {
        return -value;
    }

    DAVA::int32 sum2(DAVA::int32 a1, DAVA::int32 a2)
    {
        return a1 + a2;
    }

    DAVA::int32 sum3(DAVA::int32 a1, DAVA::int32 a2, DAVA::int32 a3)
    {
        return a1 + a2 + a3;
    }

    DAVA::int32 sum4(DAVA::int32 a1, DAVA::int32 a2, DAVA::int32 a3, DAVA::int32 a4)
    {
        return a1 + a2 + a3 + a4;
    }

    DAVA::int32 sum5(DAVA::int32 a1, DAVA::int32 a2, DAVA::int32 a3, DAVA::int32 a4, DAVA::int32 a5)
    {
        return a1 + a2 + a3 + a4 + a5;
    }

    DAVA::int32 sum6(DAVA::int32 a1, DAVA::int32 a2, DAVA::int32 a3, DAVA::int32 a4, DAVA::int32 a5, DAVA::int32 a6)
    {
        return a1 + a2 + a3 + a4 + a5 + a6;
    }

    DAVA::int32 sum7(DAVA::int32 a1, DAVA::int32 a2, DAVA::int32 a3, DAVA::int32 a4, DAVA::int32 a5, DAVA::int32 a6, DAVA::int32 a7)
    {
        return a1 + a2 + a3 + a4 + a5 + a6 + a7;
    }

public:
    DAVA::int32 intVal = 0;
    DAVA::float32 floatVal = 0.0f;
    bool boolVal = false;
    DAVA::String stringVal;
    DAVA::Color colorVal;
    ReflClass* subClass = nullptr;
};

DAVA_VIRTUAL_REFLECTION_IMPL(ReflClass)
{
    DAVA::ReflectionRegistrator<ReflClass>::Begin()
    .Field("intVal", &ReflClass::intVal)
    .Field("floatVal", &ReflClass::floatVal)
    .Field("stringVal", &ReflClass::stringVal)
    .Field("boolVal", &ReflClass::boolVal)
    .Field("colorVal", &ReflClass::colorVal)
    .Field("subClass", &ReflClass::subClass)
    .Method("returnTrue", &ReflClass::returnTrue)
    .Method("invert", &ReflClass::invert)
    .Method("sum2", &ReflClass::sum2)
    .Method("sum3", &ReflClass::sum3)
    .Method("sum4", &ReflClass::sum4)
    .Method("sum5", &ReflClass::sum5)
    .Method("sum6", &ReflClass::sum6)
    .Method("sum7", &ReflClass::sum7)
    .End();
}

DAVA_TESTCLASS (ScriptTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("LuaScript.cpp")
    DECLARE_COVERED_FILES("LuaException.cpp")
    DECLARE_COVERED_FILES("LuaBridge.cpp")
    END_FILES_COVERED_BY_TESTS();

    DAVA_TEST (DavaFunctionsTest)
    {
        DAVA::LuaScript s;

        const DAVA::String script = R"script(
-- DV functions
DV.Debug("Debug msg")
)script";

        TEST_VERIFY(s.ExecStringSafe(script) >= 0);
    }

    DAVA_TEST (ReflectionTest)
    {
        DAVA::LuaScript s;

        ReflClass subcl;
        subcl.intVal = 10;
        ReflClass cl;
        cl.intVal = 5;
        cl.floatVal = 1.4f;
        cl.boolVal = true;
        cl.stringVal = "Demo string";
        cl.colorVal = DAVA::Color::Black;
        cl.subClass = &subcl;
        DAVA::Reflection clRef = DAVA::Reflection::Create(&cl);

        const DAVA::String script = R"script(
function fields_valid(context)
    -- Get value tests
    local intVal = context.intVal
    assert(intVal == 5, "Test fail! context.intVal " .. intVal .. " != 5")
    local floatVal = context.floatVal
    assert(math.abs(floatVal - 1.4) <= 0.00001, "Test fail! context.floatVal " .. floatVal .. " != 1.4")
    local boolVal = context.boolVal
    assert(boolVal == true, "Test fail! context.boolVal " .. tostring(boolVal) .. " != true")
    local stringVal = context.stringVal
    assert(stringVal == "Demo string", "Test fail! context.stringVal '" .. stringVal .. "' != 'Demo string'")

    -- Get global value
    intVal = GlobRef.intVal
    assert(intVal == 5, "Test fail! context.intVal " .. intVal .. " != 5")

    -- Get sub reflection
    local subClass = context.subClass
    intVal = subClass.intVal
    assert(intVal == 10, "Test fail! subClass.intVal " .. intVal .. " != 10")

    -- Set value tests
    context.intVal = 42
    assert(context.intVal == 42, "Test fail! context.intVal " .. context.intVal .. " != 42")

    context.floatVal = 3.14
    assert(math.abs(context.floatVal - 3.14) <= 0.00001, "Test fail! context.floatVal " .. context.floatVal .. " != 3.14")

    context.boolVal = false
    assert(context.boolVal == false, "Test fail! context.boolVal " .. tostring(context.boolVal) .. " != false")

    context.stringVal = "New demo string"
    assert(context.stringVal == "New demo string", "Test fail! context.stringVal '" .. context.stringVal .. "' != 'New demo string'")

    -- Test complex type DAVA::Color as userdata
    --subClass.colorVal = context.colorVal
    --assert(subClass.colorVal == context.colorVal, "Test fail! subClass.colorVal (" ..  tostring(subClass.colorVal) .. ") != context.colorVal (" .. tostring(context.colorVal) .. ")")
end

function methods_valid(context)
    local ret = context.returnTrue()
    assert(ret == true, "Test fail! context.returnTrue() return '" .. tostring(ret) .. "'")
    local invertedValue = context.invert(42)
    assert(invertedValue == -42, "Test fail! context.invert(42) '" .. tostring(invertedValue) .. "' != -42")
    local sum = context.sum2(1, 2)
    assert(sum == 3, "Test fail! context.sum2 '" .. tostring(sum) .. "' != 3")
    sum = context.sum3(1, 2, 3)
    assert(sum == 6, "Test fail! context.sum3 '" .. tostring(sum) .. "' != 6")
    sum = context.sum4(1, 2, 3, 4)
    assert(sum == 10, "Test fail! context.sum4 '" .. tostring(sum) .. "' != 10")
    sum = context.sum5(1, 2, 3, 4, 5)
    assert(sum == 15, "Test fail! context.sum5 '" .. tostring(sum) .. "' != 15")
    sum = context.sum6(1, 2, 3, 4, 5, 6)
    assert(sum == 21, "Test fail! context.sum6 '" .. tostring(sum) .. "' != 21")
end

function incorect_args_count(context)
    context.returnTrue(true)
end

function over_6_args(context)
    context.sum7(1, 2, 3, 4, 5, 6, 7)
end

function wrong_arg_type(context)
    context.invert("str")
end

function get_wrong_field(context)
    local result = context.undefinedMethod
    assert(result ~= nil)
end

function call_wrong_method(context)
    context.undefinedMethod()
end

function tostring_test(context)
    assert(tostring(context.colorVal) ~= "", "Test fail! tostring(Any) is empty!")
    assert(tostring(context.invert) ~= "", "Test fail! tostring(AnyFn) is empty!")
    assert(tostring(context) ~= "", "Test fail! tostring(Reflection) is empty!")
end

function return_complex(context)
    -- Return Color, AnyFn and Reflection
    return context.colorVal, context.returnTrue, context
end

)script";

        s.SetGlobalVariable("GlobRef", clRef);

        // Compile and execute script
        TEST_VERIFY(s.ExecStringSafe(script) >= 0);

        // Call field test
        TEST_VERIFY(s.ExecFunctionSafe("fields_valid", clRef) >= 0);
        TEST_VERIFY(cl.intVal == 42);
        TEST_VERIFY(FLOAT_EQUAL(cl.floatVal, 3.14f));
        TEST_VERIFY(cl.boolVal == false);
        TEST_VERIFY(cl.stringVal == "New demo string");
        //TEST_VERIFY(cl.colorVal == subcl.colorVal);

        // Call fields error test
        TEST_VERIFY(s.ExecFunctionSafe("get_wrong_field", clRef) < 0);

        // Call methods test
        TEST_VERIFY(s.ExecFunctionSafe("methods_valid", clRef) >= 0);

        // Call methods errors tests
        TEST_VERIFY(s.ExecFunctionSafe("incorect_args_count", clRef) < 0);
        TEST_VERIFY(s.ExecFunctionSafe("over_6_args", clRef) < 0);
        TEST_VERIFY(s.ExecFunctionSafe("wrong_arg_type", clRef) < 0);
        TEST_VERIFY(s.ExecFunctionSafe("call_wrong_method", clRef) < 0);

        // to string test
        TEST_VERIFY(s.ExecFunctionSafe("tostring_test", clRef) >= 0);

        // Check return complex types
        try
        {
            DAVA::int32 nresults = s.ExecFunctionSafe("return_complex", clRef);
            TEST_VERIFY(nresults == 3);
            DAVA::Any r1 = s.GetResult(1);
            TEST_VERIFY(!r1.IsEmpty());
            //TEST_VERIFY(r1.CanGet<DAVA::Color>());
            DAVA::Any r2 = s.GetResult(2);
            TEST_VERIFY(!r2.IsEmpty());
            TEST_VERIFY(r2.CanGet<DAVA::AnyFn>());
            DAVA::Any r3 = s.GetResult(3);
            TEST_VERIFY(!r3.IsEmpty());
            TEST_VERIFY(r3.CanGet<DAVA::Reflection>());
            s.Pop(nresults);
        }
        catch (const DAVA::LuaException&)
        {
            TEST_VERIFY(false);
        }
    }

    DAVA_TEST (BasicTest)
    {
        DAVA::LuaScript s;

        const DAVA::String script = R"script(
function without_results()
    -- nothing return
end

function with_results(...)
    return ...
end

function unsupported_results()
    tbl = {}
    fn = function() return nil end
    -- th = thread?
    return tbl, fn
end

)script";

        TEST_VERIFY(s.ExecStringSafe(script) == 0);

        // Without results test
        DAVA::int32 nresults = s.ExecFunctionSafe("without_results");
        TEST_VERIFY(nresults == 0);
        // Exception
        try
        {
            s.GetResult<DAVA::int32>(1);
            TEST_VERIFY(false);
        }
        catch (const DAVA::LuaException&)
        {
            TEST_VERIFY(true);
        }
        // Safe
        DAVA::Any r;
        TEST_VERIFY(!s.GetResultSafe<DAVA::int32>(1, r));
        TEST_VERIFY(r.IsEmpty());

        // With results test without preferred types
        nresults = s.ExecFunctionSafe("with_results",
                                      DAVA::Any(),
                                      DAVA::char8(8),
                                      DAVA::char16(16),
                                      DAVA::int8(8),
                                      DAVA::int16(16),
                                      DAVA::int32(32),
                                      DAVA::int64(64),
                                      DAVA::float32(32.f),
                                      DAVA::float64(64.f),
                                      DAVA::String("string"),
                                      DAVA::WideString(L"wide string"),
                                      bool(true),
                                      DAVA::Reflection(),
                                      DAVA::Color::White);
        TEST_VERIFY(nresults == 14);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(14, r)); // Color
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::Color>());
        TEST_VERIFY(r.Get<DAVA::Color>() == DAVA::Color::White);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(13, r)); // Empty Reflection cast to nil in Lua and back cast to empty Any
        TEST_VERIFY(r.IsEmpty());
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(12, r)); // bool
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<bool>());
        TEST_VERIFY(r.Get<bool>() == true);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(11, r)); // wide string cast to UTF-8 string
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::String>());
        TEST_VERIFY(r.Get<DAVA::String>() == "wide string");
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(10, r)); // string
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::String>());
        TEST_VERIFY(r.Get<DAVA::String>() == "string");
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(9, r)); // float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 64.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(8, r)); // float32 casts as float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 32.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(7, r)); // int64 casts as float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 64.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(6, r)); // int32 casts as float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 32.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(5, r)); // int16 casts as float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 16.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(4, r)); // int8 casts as float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 8.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(3, r)); // char16 casts as float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 16.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(2, r)); // char8 casts as float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 8.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(1, r)); // nil cast to empty Any
        TEST_VERIFY(r.IsEmpty());
        s.Pop(nresults);

        // With results test with preferred types
        nresults = s.ExecFunctionSafe("with_results",
                                      DAVA::Any(),
                                      DAVA::char8(8),
                                      DAVA::char16(16),
                                      DAVA::int8(8),
                                      DAVA::int16(16),
                                      DAVA::int32(32),
                                      DAVA::int64(64),
                                      DAVA::float32(32.f),
                                      DAVA::float64(64.f),
                                      DAVA::String("string"),
                                      DAVA::WideString(L"wide string"),
                                      bool(true),
                                      DAVA::Reflection(),
                                      DAVA::Color::White);
        TEST_VERIFY(nresults == 14);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe(-1, r)); // Color
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::Color>());
        TEST_VERIFY(r.Get<DAVA::Color>() == DAVA::Color::White);
        r.Clear();
        TEST_VERIFY(!s.GetResultSafe<DAVA::Any>(-2, r)); // Empty Reflection cast to nil and can't cast to other type
        TEST_VERIFY(r.IsEmpty());
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<bool>(-3, r)); // bool
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<bool>());
        TEST_VERIFY(r.Get<bool>() == true);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::WideString>(-4, r)); // wide string
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::WideString>());
        TEST_VERIFY(r.Get<DAVA::WideString>() == L"wide string");
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::String>(-5, r)); // string
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::String>());
        TEST_VERIFY(r.Get<DAVA::String>() == "string");
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::float64>(-6, r)); // float64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float64>());
        TEST_VERIFY(FLOAT_EQUAL_EPS(r.Get<DAVA::float64>(), 64.0, 1e-7));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::float32>(-7, r)); // float32
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::float32>());
        TEST_VERIFY(FLOAT_EQUAL(r.Get<DAVA::float32>(), 32.f));
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::int64>(-8, r)); // int64
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::int64>());
        TEST_VERIFY(r.Get<DAVA::int64>() == 64);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::int32>(-9, r)); // int32
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::int32>());
        TEST_VERIFY(r.Get<DAVA::int32>() == 32);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::int16>(-10, r)); // int16
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::int16>());
        TEST_VERIFY(r.Get<DAVA::int16>() == 16);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::int8>(-11, r)); // int8
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::int8>());
        TEST_VERIFY(r.Get<DAVA::int8>() == 8);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::char16>(-12, r)); // char16
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::char16>());
        TEST_VERIFY(r.Get<DAVA::char16>() == 16);
        r.Clear();
        TEST_VERIFY(s.GetResultSafe<DAVA::char8>(-13, r)); // char8
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::char8>());
        TEST_VERIFY(r.Get<DAVA::char8>() == 8);
        r.Clear();
        TEST_VERIFY(!s.GetResultSafe<DAVA::Any>(-14, r)); // Empty Any cast to nil in Lua and can't cast to other type
        TEST_VERIFY(r.IsEmpty());
        s.Pop(nresults);

        // Unsupported type test
        nresults = s.ExecFunctionSafe("unsupported_results");
        TEST_VERIFY(nresults == 2);
        TEST_VERIFY(!s.GetResultSafe(1, r));
        TEST_VERIFY(!s.GetResultSafe(2, r));
        s.Pop(nresults);

        // Dump test
        nresults = s.ExecFunctionSafe("with_results",
                                      DAVA::Any(),
                                      DAVA::char8(8),
                                      DAVA::char16(16),
                                      DAVA::int8(8),
                                      DAVA::int16(16),
                                      DAVA::int32(32),
                                      DAVA::int64(64),
                                      DAVA::float32(32.f),
                                      DAVA::float64(64.f),
                                      DAVA::String("string"),
                                      DAVA::WideString(L"wide string"),
                                      bool(true),
                                      DAVA::Reflection(),
                                      DAVA::Color::White);
        TEST_VERIFY(nresults == 14);
        nresults += s.ExecFunctionSafe("unsupported_results");
        TEST_VERIFY(nresults == 16);
        s.DumpStackToLog(DAVA::Logger::LEVEL_DEBUG);
        s.Pop(nresults);
    }

    DAVA_TEST (CollectionReflectionTest)
    {
        DAVA::LuaScript s;
        DAVA::Vector<DAVA::int32> v{ 1, 2, 3, 4, 5 };
        DAVA::Reflection vRef = DAVA::Reflection::Create(&v);

        const DAVA::String script = R"script(
function from_vector(v, index)
    return v[index]
end

function to_vector(v, index, value)
    v[index] = value
end

)script";

        TEST_VERIFY(s.ExecStringSafe(script) == 0);

        // Read from vector by index
        DAVA::int32 nresults = s.ExecFunctionSafe("from_vector", vRef, 1);
        TEST_VERIFY(nresults == 1);
        DAVA::Any r;
        TEST_VERIFY(s.GetResultSafe<DAVA::int32>(1, r));
        TEST_VERIFY(!r.IsEmpty());
        TEST_VERIFY(r.CanGet<DAVA::int32>());
        TEST_VERIFY(r.Get<DAVA::int32>() == 1);
        s.Pop(nresults);

        // Write to vector by index
        nresults = s.ExecFunctionSafe("to_vector", vRef, 1, 42);
        TEST_VERIFY(nresults == 0);
        TEST_VERIFY(v[0] == 42);

        // Wrong key type error test
        TEST_VERIFY(s.ExecFunctionSafe("from_vector", vRef, true) < 0);
        TEST_VERIFY(s.ExecFunctionSafe("to_vector", vRef, true, 42) < 0);

        // Wrong value type error test
        TEST_VERIFY(s.ExecFunctionSafe("to_vector", vRef, 1, "string") < 0);
    }

    DAVA_TEST (CompileStringErrorTest)
    {
        DAVA::LuaScript s;

        const DAVA::String error_script = R"script(
incorrect script
)script";

        // Check exception
        try
        {
            s.ExecString(error_script);
            TEST_VERIFY(false);
        }
        catch (const DAVA::LuaException&)
        {
            TEST_VERIFY(true);
        }

        // Check safe method
        TEST_VERIFY(s.ExecStringSafe(error_script) < 0);
    }

    DAVA_TEST (ExecStringErrorTest)
    {
        DAVA::LuaScript s;

        const DAVA::String error_script = R"script(
undefined_function_call("test")
)script";

        // Check exception
        try
        {
            s.ExecString(error_script);
            TEST_VERIFY(false);
        }
        catch (const DAVA::LuaException&)
        {
            TEST_VERIFY(true);
        }

        // Check safe method
        TEST_VERIFY(s.ExecStringSafe(error_script) < 0);
    }

    DAVA_TEST (ExecFunctionErrorTest)
    {
        DAVA::LuaScript s;

        const DAVA::String error_script = R"script(
function main()
    undefined_function_call("test")
end
)script";

        TEST_VERIFY(s.ExecStringSafe(error_script) >= 0);

        // Check exception
        try
        {
            s.ExecFunction("main");
            TEST_VERIFY(false);
        }
        catch (const DAVA::LuaException&)
        {
            TEST_VERIFY(true);
        }

        // Check safe method
        TEST_VERIFY(s.ExecFunctionSafe("main") < 0);
    }

    DAVA_TEST (MoveConstructorTest)
    {
        DAVA::LuaScript s;

        const DAVA::String easy_script = R"script(
function main()
end
)script";

        TEST_VERIFY(s.ExecStringSafe(easy_script) >= 0);

        DAVA::LuaScript moveScript(std::move(s));

        TEST_VERIFY(moveScript.ExecFunctionSafe("main") >= 0);
    }

    DAVA_TEST (LuaExceptionTest)
    {
        try
        {
            DAVA_THROW(DAVA::LuaException, -1, DAVA::String("Some error"));
            TEST_VERIFY(false);
        }
        catch (const DAVA::LuaException& e)
        {
            TEST_VERIFY(e.ErrorCode() == -1);
        }

        try
        {
            DAVA_THROW(DAVA::LuaException, -1, static_cast<const char*>("Some error"));
            TEST_VERIFY(false);
        }
        catch (const DAVA::LuaException& e)
        {
            TEST_VERIFY(e.ErrorCode() == -1);
        }
    }
};