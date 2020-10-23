#include "Base/Result.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (ResultTest)
{
    DAVA_TEST (GetResultFunction)
    {
        TEST_VERIFY(GetResultFunction(Result::RESULT_SUCCESS));
        TEST_VERIFY(GetResultFunction(Result::RESULT_WARNING));
        TEST_VERIFY(!GetResultFunction(Result::RESULT_ERROR));

        TEST_VERIFY(GetResultFunction(Result::RESULT_SUCCESS).IsSuccess());
        TEST_VERIFY(GetResultFunction(Result::RESULT_WARNING).IsSuccess());
        TEST_VERIFY(!GetResultFunction(Result::RESULT_ERROR).IsSuccess());

        Deque<Result> results;
        results.emplace_back(Result::RESULT_SUCCESS, "this is ");
        results.emplace_back(Result::RESULT_WARNING, "result ");
        results.emplace_back(Result::RESULT_ERROR, "test.");
        ResultList resultList;
        for (const auto& result : results)
        {
            resultList.AddResultList(GetResultFunction(result));
        }
        TEST_VERIFY(resultList.GetResults().size() == results.size());
        auto resultIt = resultList.GetResults().begin();
        for (const auto& result : results)
        {
            TEST_VERIFY(result == *resultIt++);
        }
    }

    ResultList GetResultFunction(const Result& result)
    {
        return ResultList(result);
    }

    ResultList GetResultFunction(const Result&& result)
    {
        return ResultList(result);
    }
}
;
