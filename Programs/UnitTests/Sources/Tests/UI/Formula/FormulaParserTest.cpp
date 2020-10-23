#include "DAVAEngine.h"

#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaFormatter.h"

#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (FormulaParserTest)
{
    class FormulaExpressionTraversalAlg : FormulaExpressionVisitor
    {
    public:
        ~FormulaExpressionTraversalAlg()
        {
        }

        void Traverse(FormulaExpression* exp)
        {
            exp->Accept(this);
        }

        void Visit(FormulaValueExpression* exp) override
        {
            res.push_back(FormulaFormatter::AnyToString(exp->GetValue()));
        }

        void Visit(FormulaNegExpression* exp) override
        {
            res.push_back("-");
            exp->GetExp()->Accept(this);
        }

        void Visit(FormulaNotExpression* exp) override
        {
            res.push_back("not");
            exp->GetExp()->Accept(this);
        }

        void Visit(FormulaWhenExpression* exp) override
        {
            res.push_back("when");
            for (auto& it : exp->GetBranches())
            {
                res.push_back("cond");
                it.first->Accept(this);
                res.push_back("exp");
                it.second->Accept(this);
            }
            res.push_back("else");
            exp->GetElseBranch()->Accept(this);
        }

        void Visit(FormulaBinaryOperatorExpression* exp) override
        {
            res.push_back(FormulaFormatter::BinaryOpToString(exp->GetOperator()));
            exp->GetLhs()->Accept(this);
            exp->GetRhs()->Accept(this);
        }

        void Visit(FormulaFunctionExpression* exp) override
        {
            res.push_back("func_" + exp->GetName());

            for (const std::shared_ptr<FormulaExpression>& paramExp : exp->GetParms())
            {
                paramExp->Accept(this);
            }
        }

        void Visit(FormulaFieldAccessExpression* exp) override
        {
            res.push_back("field_" + exp->GetFieldName());
            if (exp->GetExp())
            {
                exp->GetExp()->Accept(this);
            }
        }

        void Visit(FormulaIndexExpression* exp) override
        {
            res.push_back("index");
            exp->GetExp()->Accept(this);
            exp->GetIndexExp()->Accept(this);
        }

        Vector<String> res;
    };

    void TraverseMap(const std::shared_ptr<FormulaDataMap>& map, Vector<String>& res)
    {
        res.push_back("map");
        const Vector<String>& keys = map->GetOrderedKeys();
        for (String key : keys)
        {
            res.push_back("key_" + key);
            ProcessValue(map->Find(key), res);
        }
    }

    void TraverseVector(const std::shared_ptr<FormulaDataVector>& vector, Vector<String>& res)
    {
        res.push_back("vector");
        for (size_t i = 0; i < vector->GetCount(); i++)
        {
            ProcessValue(vector->Get(i), res);
        }
    }

    void ProcessValue(const Any& val, Vector<String>& res)
    {
        if (val.CanCast<std::shared_ptr<FormulaExpression>>())
        {
            FormulaExpressionTraversalAlg alg;
            alg.Traverse(val.Cast<std::shared_ptr<FormulaExpression>>().get());
            for (String s : alg.res)
            {
                res.push_back(s);
            }
        }
        else if (val.CanCast<std::shared_ptr<FormulaDataMap>>())
        {
            std::shared_ptr<FormulaDataMap> map = val.Cast<std::shared_ptr<FormulaDataMap>>();
            TraverseMap(map, res);
        }
        else if (val.CanCast<std::shared_ptr<FormulaDataVector>>())
        {
            std::shared_ptr<FormulaDataVector> vector = val.Cast<std::shared_ptr<FormulaDataVector>>();
            TraverseVector(vector, res);
        }
        else
        {
            res.push_back(FormulaFormatter::AnyToString(val));
        }
    }

    Vector<String> Parse(const String& str)
    {
        std::shared_ptr<FormulaExpression> exp = FormulaParser(str).ParseExpression();
        FormulaExpressionTraversalAlg alg;
        alg.Traverse(exp.get());
        return alg.res;
    }

    void SetUp(const String& testName) override
    {
    }

    void TearDown(const String& testName) override
    {
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParseBinaryOperations)
    {
        TEST_VERIFY(Parse("1 + 2") == Vector<String>({ "+", "1", "2" }));
        TEST_VERIFY(Parse("1 - 2") == Vector<String>({ "-", "1", "2" }));
        TEST_VERIFY(Parse("1 * 2") == Vector<String>({ "*", "1", "2" }));
        TEST_VERIFY(Parse("5 * (1 + 2)") == Vector<String>({ "*", "5", "+", "1", "2" }));
        TEST_VERIFY(Parse("1 / 2") == Vector<String>({ "/", "1", "2" }));
        TEST_VERIFY(Parse("1 % 2") == Vector<String>({ "%", "1", "2" }));
        TEST_VERIFY(Parse("true and false") == Vector<String>({ "and", "true", "false" }));
        TEST_VERIFY(Parse("true or false") == Vector<String>({ "or", "true", "false" }));
        TEST_VERIFY(Parse("true == false") == Vector<String>({ "==", "true", "false" }));
        TEST_VERIFY(Parse("true != false") == Vector<String>({ "!=", "true", "false" }));
        TEST_VERIFY(Parse("a <= b") == Vector<String>({ "<=", "field_a", "field_b" }));
        TEST_VERIFY(Parse("a < b") == Vector<String>({ "<", "field_a", "field_b" }));
        TEST_VERIFY(Parse("a >= b") == Vector<String>({ ">=", "field_a", "field_b" }));
        TEST_VERIFY(Parse("a > b") == Vector<String>({ ">", "field_a", "field_b" }));
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParseUnaryOperations)
    {
        TEST_VERIFY(Parse("-a") == Vector<String>({ "-", "field_a" }));
        TEST_VERIFY(Parse("not b") == Vector<String>({ "not", "field_b" }));
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParseWhenOperations)
    {
        TEST_VERIFY(Parse("when true -> 1, 0") == Vector<String>({ "when", "cond", "true", "exp", "1", "else", "0" }));
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParsePriorities)
    {
        TEST_VERIFY(Parse("1 + 2 - 3") == Vector<String>({ "-", "+", "1", "2", "3" }));
        TEST_VERIFY(Parse("1+2-3") == Vector<String>({ "-", "+", "1", "2", "3" }));
        TEST_VERIFY(Parse("1---3") == Vector<String>({ "-", "1", "-", "-", "3" }));
        TEST_VERIFY(Parse("1 * 2 / 3") == Vector<String>({ "/", "*", "1", "2", "3" }));
        TEST_VERIFY(Parse("1 + 2 / 3") == Vector<String>({ "+", "1", "/", "2", "3" }));
        TEST_VERIFY(Parse("1 + 2 < 3 - 4") == Vector<String>({ "<", "+", "1", "2", "-", "3", "4" }));
        TEST_VERIFY(Parse("1 < 2 == 3 - 4") == Vector<String>({ "==", "<", "1", "2", "-", "3", "4" }));
        TEST_VERIFY(Parse("1 < 2 and 3 == 4") == Vector<String>({ "and", "<", "1", "2", "==", "3", "4" }));
        TEST_VERIFY(Parse("true or false and true") == Vector<String>({ "or", "true", "and", "false", "true" }));
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParseFunction)
    {
        TEST_VERIFY(Parse("foo(a + c, d)") == Vector<String>({ "func_foo", "+", "field_a", "field_c", "field_d" }));
        TEST_VERIFY(Parse("foo()") == Vector<String>({ "func_foo" }));
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParseField)
    {
        TEST_VERIFY(Parse("var") == Vector<String>({ "field_var" }));
        TEST_VERIFY(Parse("a.b") == Vector<String>({ "field_b", "field_a" }));
        Vector<String> Res = Parse("a.b.c");
        TEST_VERIFY(Res == Vector<String>({ "field_c", "field_b", "field_a" }));
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParseIndex)
    {
        TEST_VERIFY(Parse("var[25]") == Vector<String>({ "index", "field_var", "25" }));
        TEST_VERIFY(Parse("a.var[25 + 5]") == Vector<String>({ "index", "field_var", "field_a", "+", "25", "5" }));
    }

    // FormulaParser::ParseMap
    DAVA_TEST (ParseMap)
    {
        std::shared_ptr<FormulaDataMap> map = FormulaParser("a = exp1 + 3;"
                                                            "b = {a = 2};"
                                                            "c = [1; 2; 3];")
                                              .ParseMap();

        Vector<String> res;
        TraverseMap(map, res);

        TEST_VERIFY(res == Vector<String>(
                           { "map",
                             "key_a", "+", "field_exp1", "3",
                             "key_b", "map", "key_a", "2",
                             "key_c", "vector", "1", "2", "3"
                           }));
    }

    // FormulaParser::ParseExpression
    DAVA_TEST (ParseExpressionWithErrors)
    {
        bool wasException = false;
        try
        {
            Parse("a + ");
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }

        TEST_VERIFY(wasException == true);

        wasException = false;
        try
        {
            Parse("+ ad + (4 * 5");
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }

        TEST_VERIFY(wasException == true);
        wasException = false;
        try
        {
            Parse("foo(a b)");
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }

        TEST_VERIFY(wasException == true);
    }
};
