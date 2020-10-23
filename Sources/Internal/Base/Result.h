#ifndef __DAVAENGINE_RESULT_H_
#define __DAVAENGINE_RESULT_H_

#include "Base/BaseTypes.h"
#include "FileSystem/VariantType.h"

namespace DAVA
{
struct Result
{
    enum ResultType
    {
        RESULT_SUCCESS,
        RESULT_WARNING,
        RESULT_ERROR
    };
    Result(const ResultType type = RESULT_SUCCESS, const String& message = String());
    Result(const Result& result) = default;
    Result(Result&& result);
    Result& operator=(const Result& result) = default;
    Result& operator=(Result&& result);
    operator bool() const;
    ResultType type = RESULT_SUCCESS;
    String message;
};

inline Result::operator bool() const
{
    return type == RESULT_SUCCESS || type == RESULT_WARNING;
}

class ResultList
{
public:
    explicit ResultList();
    explicit ResultList(const Result& result);
    ResultList(Result&& result);
    ResultList(const ResultList& resultList) = default;
    ResultList(ResultList&& resultList);
    ~ResultList() = default;
    operator bool() const;
    bool IsSuccess() const;
    bool HasErrors() const;
    bool HasWarnings() const;
    ResultList& operator=(const ResultList& resultList) = default;
    ResultList& operator=(ResultList&& resultList);
    ResultList& operator<<(const Result& result);
    ResultList& operator<<(Result&& result);
    ResultList& AddResult(const Result& result);
    ResultList& AddResult(Result&& result);
    ResultList& AddResult(const Result::ResultType type = Result::RESULT_SUCCESS, const String& message = String());
    ResultList& AddResultList(const ResultList& resultList);
    ResultList& AddResultList(ResultList&& resultList);

    const Deque<Result>& GetResults() const;
    String GetResultMessages() const;

private:
    bool hasErrors = false;
    bool hasWarnings = false;
    Deque<Result> results;
};

inline ResultList::operator bool() const
{
    return !hasErrors;
}

inline bool ResultList::IsSuccess() const
{
    return !hasErrors;
}

inline bool ResultList::HasErrors() const
{
    return hasErrors;
}

inline bool ResultList::HasWarnings() const
{
    return hasWarnings;
}

inline const Deque<Result>& ResultList::GetResults() const
{
    return results;
}
}
#endif // __DAVAENGINE_RESULT_H_
