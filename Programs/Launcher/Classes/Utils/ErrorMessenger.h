#ifndef ERRORVALIDATOR_H
#define ERRORVALIDATOR_H

#include "defines.h"
#include <QString>

namespace ErrorMessenger
{
enum ErrorID
{
    ERROR_DOC_ACCESS = 0,
    ERROR_NETWORK,
    ERROR_CONFIG,
    ERROR_UNPACK,
    ERROR_IS_RUNNING,
    ERROR_UPDATE,
    ERROR_PATH,
    ERROR_FILE,
    ERROR_COUNT
};
void ShowErrorMessage(ErrorID id, const QString& addInfo);
void ShowErrorMessage(ErrorID id, int errorCode = 0, const QString& addInfo = "");

void LogMessage(QtMsgType, const QString& msg);
};

#endif // ERRORVALIDATOR_H
