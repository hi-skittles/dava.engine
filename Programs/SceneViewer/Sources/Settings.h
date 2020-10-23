#pragma once

#include <Base/StaticSingleton.h>
#include <Base/BaseTypes.h>
#include <FileSystem/VariantType.h>
#include <FileSystem/KeyedArchive.h>
#include <Base/ScopedPtr.h>

class Settings
{
public:
    Settings();

    void Load();
    void Save();

    DAVA::FilePath GetLastOpenedScenePath() const;
    void SetLastOpenedScenePath(const DAVA::FilePath&);

    const DAVA::KeyedArchive* GetQualitySettings() const;
    void SetQualitySettings(DAVA::KeyedArchive*);

private:
    DAVA::VariantType* GetValue(const DAVA::String& path) const;
    void SetValue(const DAVA::String& path, const DAVA::VariantType& value);

private:
    DAVA::ScopedPtr<DAVA::KeyedArchive> settings;
};
