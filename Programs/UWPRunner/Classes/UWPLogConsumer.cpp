#include "UWPLogConsumer.h"

UWPLogConsumer::UWPLogConsumer()
{
    channelOpened = IsChannelOpen();
    newDataNotifier.Connect(this, &UWPLogConsumer::OnNewData);
}

bool UWPLogConsumer::IsSessionEnded()
{
    return !channelOpened && HasReceivedData();
}

bool UWPLogConsumer::HasReceivedData()
{
    return dataReceived;
}

void UWPLogConsumer::ChannelOpen()
{
    channelOpened = true;
}

void UWPLogConsumer::ChannelClosed(const DAVA::char8* message)
{
    channelOpened = false;
}

void UWPLogConsumer::OnNewData(const DAVA::String& str)
{
    dataReceived = true;
    newMessageNotifier.Emit(str);
}