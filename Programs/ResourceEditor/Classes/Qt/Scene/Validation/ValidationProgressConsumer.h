#pragma once

#include "Base/BaseTypes.h"
#include "Logger/Logger.h"

class ValidationProgressConsumer
{
public:
    virtual void ValidationStarted(const DAVA::String& validationTitle){};
    virtual void ValidationAlert(const DAVA::String& alertMessage){};
    virtual void ValidationDone(){};
};

class ValidationProgressToLog : public ValidationProgressConsumer
{
protected:
    void ValidationStarted(const DAVA::String& title) override;
    void ValidationAlert(const DAVA::String& alertMessage) override;
    void ValidationDone() override;

private:
    DAVA::String validationTitle;
};

inline void ValidationProgressToLog::ValidationStarted(const DAVA::String& title)
{
    validationTitle = title;
}

inline void ValidationProgressToLog::ValidationAlert(const DAVA::String& alertMessage)
{
    DAVA::Logger::Warning("%s: %s", validationTitle.c_str(), alertMessage.c_str());
}

inline void ValidationProgressToLog::ValidationDone()
{
    DAVA::Logger::Info("%s: done ", validationTitle.c_str());
}
