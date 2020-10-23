#pragma once

#include <Base/BaseTypes.h>
#include <Base/Result.h>

class ValidationProgressConsumer;
class ValidationProgress
{
public:
    void SetProgressConsumer(ValidationProgressConsumer* c);

    void Started(const DAVA::String& title);
    void Alerted(const DAVA::String& msg);
    void Finished();

    DAVA::Result GetResult() const;

private:
    DAVA::Result result = DAVA::Result::RESULT_SUCCESS;
    ValidationProgressConsumer* consumer = nullptr;
};

inline void ValidationProgress::SetProgressConsumer(ValidationProgressConsumer* c)
{
    consumer = c;
}

inline DAVA::Result ValidationProgress::GetResult() const
{
    return result;
}
