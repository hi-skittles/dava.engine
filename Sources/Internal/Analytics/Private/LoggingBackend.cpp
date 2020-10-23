#include "Analytics/LoggingBackend.h"
#include "Logger/Logger.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
namespace Analytics
{
String FormatDateTime(const DateTime& dt)
{
    int32 year = dt.GetYear();
    int32 month = dt.GetMonth();
    int32 day = dt.GetDay();
    int32 hour = dt.GetHour();
    int32 minute = dt.GetMinute();
    int32 sec = dt.GetSecond();

    return Format("%04d-%02d-%02d %02d:%02d:%02d%", year, month, day, hour, minute, sec);
}

LoggingBackend::LoggingBackend(const FilePath& path)
    : filePath(path)
{
}

void LoggingBackend::ConfigChanged(const KeyedArchive& config)
{
}

void LoggingBackend::ProcessEvent(const AnalyticsEvent& event)
{
    String value;
    String msg;
    msg.reserve(256);

    msg = FormatDateTime(event.timestamp);
    msg += " Name: " + event.name + " ";

    bool firstField = true;
    for (const auto& field : event.fields)
    {
        if (field.second.CanCast<String>())
        {
            value = field.second.Cast<String>();
        }
        else if (field.second.CanGet<const char*>())
        {
            value = field.second.Get<const char*>();
        }
        else
        {
            continue;
        }

        if (!firstField)
        {
            msg += ", ";
        }
        msg += field.first + ": " + value;
        firstField = false;
    }

    if (filePath.IsEmpty())
    {
        Logger::Info(msg.c_str());
    }
    else
    {
        Logger::InfoToFile(filePath, msg.c_str());
    }
}

} // namespace Analytics
} // namespace DAVA
