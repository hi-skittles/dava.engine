#include "DAVAEngine.h"

#include "UI/Formula/Formula.h"

#include "Reflection/ReflectionRegistrator.h"

#include "UnitTests/UnitTests.h"

using namespace DAVA;

class FormulaTestData : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(FormulaTestData);

public:
    int intVal = 42;

    FormulaTestData()
    {
    }

    int sum(const std::shared_ptr<FormulaContext>& context, int a, int b)
    {
        return a + b;
    }
};

DAVA_VIRTUAL_REFLECTION_IMPL(FormulaTestData)
{
    ReflectionRegistrator<FormulaTestData>::Begin()
    .Field("val", &FormulaTestData::intVal)
    .Method("sum", &FormulaTestData::sum)
    .End();
};

DAVA_TESTCLASS (FormulaTest)
{
    // Formula::Parse
    DAVA_TEST (ParseAndCalculate)
    {
        FormulaTestData data;
        Formula formula;
        TEST_VERIFY(formula.Parse("  2 *sum(val , 8)  ") == true);
        TEST_VERIFY(formula.Calculate(Reflection::Create(&data)) == Any(100));
        TEST_VERIFY(formula.ToString() == "2 * sum(val, 8)");
    }

    // Formula::Parse
    DAVA_TEST (ParseErrors)
    {
        Formula formula;
        TEST_VERIFY(formula.Parse("5 +") == false);
        TEST_VERIFY(formula.GetParsingError().empty() == false);
    }

    // Formula::Calculate
    DAVA_TEST (CalculationErrors)
    {
        FormulaTestData data;
        Formula formula;
        TEST_VERIFY(formula.Parse("5 + v2") == true);
        TEST_VERIFY(formula.Calculate(Reflection::Create(&data)) == Any());
        TEST_VERIFY(formula.GetCalculationError().empty() == false);
    }
};
