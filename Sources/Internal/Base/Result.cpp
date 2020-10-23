#include "Base/Result.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
Result::Result(ResultType type_, const DAVA::String& message_)
    : type(type_)
    , message(message_)
{
}

Result::Result(Result&& result)
    : type(result.type)
    , message(std::move(result.message))
{
    result.type = RESULT_SUCCESS;
}

Result& Result::operator=(Result&& result)
{
    if (this != &result)
    {
        type = result.type;
        message = std::move(result.message);
        result.type = RESULT_SUCCESS;
    }
    return *this;
}

ResultList::ResultList()
{
}

ResultList::ResultList(const Result& result)
    : hasErrors(result.type == Result::RESULT_ERROR)
    , hasWarnings(result.type == Result::RESULT_WARNING)
{
    results.push_back(result);
}

ResultList::ResultList(Result&& result)
{
    AddResult(std::move(result));
}

ResultList::ResultList(ResultList&& resultList)
    : hasErrors(resultList.hasErrors)
    , hasWarnings(resultList.hasWarnings)
    , results(std::move(resultList.results))
{
}

ResultList& ResultList::operator=(ResultList&& resultList)
{
    if (this != &resultList)
    {
        hasErrors = resultList.hasErrors;
        hasWarnings = resultList.hasWarnings;
        results = std::move(resultList.results);
    }
    return *this;
}

ResultList& ResultList::operator<<(const Result& result)
{
    return AddResult(std::move(result));
}

ResultList& ResultList::operator<<(Result&& result)
{
    return AddResult(std::move(result));
}

ResultList& ResultList::AddResult(const Result& result)
{
    hasErrors |= result.type == Result::RESULT_ERROR;
    hasWarnings |= result.type == Result::RESULT_WARNING;
    results.push_back(result);
    return *this;
}

ResultList& ResultList::AddResult(Result&& result)
{
    hasErrors |= result.type == Result::RESULT_ERROR;
    hasWarnings |= result.type == Result::RESULT_WARNING;
    results.emplace_back(std::move(result));
    return *this;
}

ResultList& ResultList::AddResult(const Result::ResultType type, const String& message)
{
    return AddResult(Result(type, message));
}

ResultList& ResultList::AddResultList(const ResultList& resultList)
{
    hasErrors |= resultList.hasErrors;
    hasWarnings |= resultList.hasWarnings;
    results.insert(results.end(), resultList.results.begin(), resultList.results.end());
    return *this;
}

ResultList& ResultList::AddResultList(ResultList&& resultList)
{
    DVASSERT(this != &resultList);
    hasErrors |= resultList.hasErrors;
    hasWarnings |= resultList.hasWarnings;
    if (results.empty())
    {
        results = std::move(resultList.results);
    }
    else
    {
        std::move(std::begin(resultList.results), std::end(resultList.results), std::back_inserter(results));
        resultList.results.clear();
    }
    return *this;
}

String ResultList::GetResultMessages() const
{
    StringStream stream;
    bool first = true;
    for (const Result& result : results)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            stream << std::endl;
        }
        stream << result.message;
    }
    return stream.str();
}
}