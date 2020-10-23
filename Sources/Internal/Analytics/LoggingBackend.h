#pragma once

#include "Analytics/Analytics.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
namespace Analytics
{
/**
Simple analytics events logger. Log event to console and file (if set).
Example:
\code
auto backend = std::make_unique<Analytics::LoggingBackend>("~doc:/AnalyticsLog.txt");
GetCore().AddBackend("LoggingBackend", std::move(backend));
\endcode
*/
class LoggingBackend : public IBackend
{
public:
    LoggingBackend(const FilePath& path = FilePath());

    /** Do nothing on config changing */
    void ConfigChanged(const KeyedArchive& config) override;

    /** Format message and write it to console and file (if set). */
    void ProcessEvent(const AnalyticsEvent& event) override;

private:
    FilePath filePath;
};

} // namespace Analytics
} // namespace DAVA
