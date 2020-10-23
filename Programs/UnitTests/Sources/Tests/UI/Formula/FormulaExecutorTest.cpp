#include "DAVAEngine.h"

#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaExecutor.h"
#include "UI/Formula/Private/FormulaException.h"
#include "UI/Formula/Private/FormulaFormatter.h"

#include "Reflection/ReflectionRegistrator.h"

#include "UnitTests/UnitTests.h"

using namespace DAVA;

class TestData : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(TestData);

public:
    float flVal = 0.0f;
    bool bVal = false;
    String strVal = "Hello, world";
    int intVal = 42;
    Vector<int> array;
    Map<String, int> map;

    TestData()
    {
        array.push_back(10);
        array.push_back(20);
        array.push_back(30);

        map["a"] = 11;
        map["b"] = 22;
        map["c"] = 33;
    }

    int sum(const std::shared_ptr<FormulaContext>& context, int a, int b)
    {
        return a + b;
    }

    String intToStr(const std::shared_ptr<FormulaContext>& context, int a)
    {
        return Format("*%d*", a);
    }

    String boolToStr(const std::shared_ptr<FormulaContext>& context, bool a)
    {
        return a ? "+" : "-";
    }

    String floatToStr(const std::shared_ptr<FormulaContext>& context, float a)
    {
        double var = static_cast<double>(a);
        return Format("%.3f", var);
    }
};

DAVA_VIRTUAL_REFLECTION_IMPL(TestData)
{
    ReflectionRegistrator<TestData>::Begin()
    .Field("fl", &TestData::flVal)
    .Field("b", &TestData::bVal)
    .Field("str", &TestData::strVal)
    .Field("intVal", &TestData::intVal)
    .Field("array", &TestData::array)
    .Field("map", &TestData::map)

    .Method("sum", &TestData::sum)
    .Method("intToStr", &TestData::intToStr)
    .Method("floatToStr", &TestData::floatToStr)
    .Method("boolToStr", &TestData::boolToStr)
    .End();
};

DAVA_TESTCLASS (FormulaExecutorTest)
{
    // FormulaExecutor::Calculate
    DAVA_TEST (CalculateNumbers)
    {
        TEST_VERIFY(Execute("5") == Any(5));
        TEST_VERIFY(Execute("5 + 5") == Any(10));
        TEST_VERIFY(Execute("7-2") == Any(5));
        TEST_VERIFY(Execute("7U-2U") == Any(5U));
        TEST_VERIFY(Execute("7L-9L").Get<int64>() == static_cast<int64>(-2L));
        TEST_VERIFY(Execute("7U-9U").Get<uint32>() == static_cast<uint32>(-2));
        TEST_VERIFY(Execute("-2") == Any(-2));
        TEST_VERIFY(Execute("--2") == Any(2));
        TEST_VERIFY(Execute("1---2") == Any(-1));

        TEST_VERIFY(FLOAT_EQUAL(Execute("-5.5").Get<float32>(), -5.5f));
        TEST_VERIFY(FLOAT_EQUAL(Execute("5 + 5.5").Get<float32>(), 10.5f));
        TEST_VERIFY(FLOAT_EQUAL(Execute("2.0 * 5.5").Get<float32>(), 11.0f));
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (CalculateBools)
    {
        TEST_VERIFY(Execute("not true") == Any(false));
        TEST_VERIFY(Execute("not false") == Any(true));

        TEST_VERIFY(Execute("5 > 5") == Any(false));
        TEST_VERIFY(Execute("6 > 5") == Any(true));
        TEST_VERIFY(Execute("5 >= 5") == Any(true));
        TEST_VERIFY(Execute("5 >= 4") == Any(true));

        TEST_VERIFY(Execute("5 < 5") == Any(false));
        TEST_VERIFY(Execute("6 < 7") == Any(true));
        TEST_VERIFY(Execute("5 <= 5") == Any(true));
        TEST_VERIFY(Execute("5 <= 4") == Any(false));
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (CalculateStrings)
    {
        TEST_VERIFY(Execute("\"Hello, world\" == str") == Any(true));
        TEST_VERIFY(Execute("\"Hello,\" + \" world\" == str") == Any(true));
        TEST_VERIFY(Execute("intToStr(5) == \"*5*\"") == Any(true));
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (CalculateWhen)
    {
        TEST_VERIFY(Execute("when true -> 0, 1") == Any(0));
        TEST_VERIFY(Execute("when 5 == 2 -> 0, 1") == Any(1));
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (CalculateNumbersWithErrors)
    {
        try
        {
            Execute("5 + 5L");
            TEST_VERIFY(false);
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 3] Operator '+' cannot be applied to 'int32', 'int64'");
        }

        try
        {
            Execute("5L + 5UL");
            TEST_VERIFY(false);
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 4] Operator '+' cannot be applied to 'int64', 'uint64'");
        }

        try
        {
            Execute("5L + \"543543\"");
            TEST_VERIFY(false);
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 4] Operator '+' cannot be applied to 'int64', 'String'");
        }

        try
        {
            Execute("\"54354\" - \"543543\"");
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 9] Operator '-' cannot be applied to 'String', 'String'");
        }

        try
        {
            Execute("false * true");
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 7] Operator '*' cannot be applied to 'bool', 'bool'");
        }

        try
        {
            Execute("not 5");
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 1] Invalid argument type 'int32' to unary 'not' expression");
        }

        try
        {
            Execute("-true");
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 1] Invalid argument type 'bool' to unary '-' expression");
        }
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (FieldAccess)
    {
        TEST_VERIFY(Execute("map.a") == Any(11));
        TEST_VERIFY(Execute("map.b") == Any(22));
        TEST_VERIFY(Execute("intVal") == Any(42));
        TEST_VERIFY(Execute("map.b + intVal") == Any(22 + 42));
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (FieldAccessWithErrors)
    {
        try
        {
            Execute("map.d");
            TEST_VERIFY(false);
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 5] Can't resolve symbol 'd'");
        }
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (IndexAccess)
    {
        TEST_VERIFY(Execute("array[1]") == Any(20));
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (IndexAccessWithErrors)
    {
        try
        {
            Execute("array[5.5]");
            TEST_VERIFY(false);
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 6] Can't get data 'array[5.500000]' by index '5.500000' with type 'float32'");
        }
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (Function)
    {
        TEST_VERIFY(Execute("sum(16, intVal * 2)") == Any(100));
        TEST_VERIFY(Execute("intToStr(55)") == Any(String("*55*")));
        TEST_VERIFY(Execute("floatToStr(55)") == Any(String("55.000")));
        TEST_VERIFY(Execute("floatToStr(55.5)") == Any(String("55.500")));
        TEST_VERIFY(Execute("boolToStr(true)") == Any(String("+")));
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (FunctionWithErrors)
    {
        try
        {
            Execute("sum(1, 2, 3)");
            TEST_VERIFY(false);
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 1] Can't resolve function 'sum(int32, int32, int32)'");
        }

        try
        {
            Execute("floatToStr(true)");
            TEST_VERIFY(false);
        }
        catch (const FormulaException& error)
        {
            TEST_VERIFY(error.GetFormattedMessage() == "[1, 1] Can't resolve function 'floatToStr(bool)'");
        }
    }

    // FormulaExecutor::Calculate
    DAVA_TEST (Dependencies)
    {
        TestData data;

        Vector<void*> dependencies;

        dependencies = GetDependencies("sum(16, intVal * 2)", &data);
        TEST_VERIFY(dependencies == Vector<void*>({ &(data.intVal) }));

        dependencies = GetDependencies("map.b + fl", &data);
        TEST_VERIFY(dependencies == Vector<void*>({ &(data.map), &(data.map["b"]), &(data.flVal) }));

        dependencies = GetDependencies("b and (array[1] == 1)", &data);
        TEST_VERIFY(dependencies == Vector<void*>({ &(data.bVal), &(data.array), &(data.array[1]) }));
    }

    Any Execute(const String& str)
    {
        TestData data;
        std::shared_ptr<FormulaContext> context = std::make_shared<FormulaReflectionContext>(Reflection::Create(&data), std::shared_ptr<FormulaContext>());
        FormulaExecutor executor(context);
        FormulaParser parser(str);
        std::shared_ptr<FormulaExpression> exp = parser.ParseExpression();
        Any res = executor.Calculate(exp.get());
        Logger::Debug("test: %s", FormulaFormatter::AnyToString(res).c_str());
        return res;
    }

    Vector<void*> GetDependencies(const String str, TestData* data)
    {
        std::shared_ptr<FormulaContext> context = std::make_shared<FormulaReflectionContext>(Reflection::Create(&data), std::shared_ptr<FormulaContext>());
        FormulaExecutor executor(context);
        FormulaParser parser(str);
        std::shared_ptr<FormulaExpression> exp = parser.ParseExpression();

        executor.Calculate(exp.get());
        return executor.GetDependencies();
    }
};
