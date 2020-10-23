#pragma once

#ifndef __DAVA_ENGINE_SETTINGS_IMPL__
#include "Engine/EngineSettings.h"
#endif

namespace DAVA
{
template <EngineSettings::eSetting ID>
inline const Any& EngineSettings::GetSetting() const
{
    return setting[ID];
}

template <EngineSettings::eSetting ID>
inline void EngineSettings::SetSetting(const Any& value)
{
    DVASSERT(setting[ID].GetType() == value.GetType());
    if (setting[ID] != value)
    {
        setting[ID] = value;
        settingChanged.Emit(ID);
    }
}

template <typename T>
EngineSettings::SettingRange<T>::SettingRange(const T& _min, const T& _max)
    : min(_min)
    , max(_max)
{
}

template <EngineSettings::eSetting ID, typename T>
inline const T& EngineSettings::GetSettingRefl() const
{
    const Any& v = GetSetting<ID>();
    return v.Get<T>();
}

template <EngineSettings::eSetting ID, typename T>
inline void EngineSettings::SetSettingRefl(const T& value)
{
    SetSetting<ID>(value);
}

} //ns DAVA