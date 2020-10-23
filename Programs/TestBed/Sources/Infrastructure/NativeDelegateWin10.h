#pragma once

#include <Base/BaseTypes.h>

#if defined(__DAVAENGINE_WIN_UAP__)

#include <Engine/PlatformApiWin10.h>

struct NativeDelegateWin10 : public DAVA::PlatformApi::Win10::XamlApplicationListener
{
    void OnLaunched(::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^ args) override;
    void OnActivated(::Windows::ApplicationModel::Activation::IActivatedEventArgs ^ args) override;
};

#endif // __DAVAENGINE_WIN_UAP__
