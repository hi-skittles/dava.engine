#pragma once

#include "ApplicationSettings.h"

namespace SharedDataParser
{
DAVA::List<SharedPoolParams> ParsePoolsReply(const QByteArray& data);
DAVA::List<SharedServerParams> ParseServersReply(const QByteArray& data);
ServerID ParseAddReply(const QByteArray& data);
}
