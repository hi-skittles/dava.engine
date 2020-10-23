#pragma once

#include "Base/BaseTypes.h"
#include <LoggerService/LogConsumer.h>

class UWPLogConsumer : public DAVA::Net::LogConsumer
{
public:
    UWPLogConsumer();

    bool IsSessionEnded();
    bool HasReceivedData();
    DAVA::Signal<const DAVA::String&> newMessageNotifier;

private:
    //NetService method implementation
    void ChannelOpen() override;
    void ChannelClosed(const DAVA::char8* message) override;

    void OnNewData(const DAVA::String& str);

    bool channelOpened;
    bool dataReceived = false;
};
