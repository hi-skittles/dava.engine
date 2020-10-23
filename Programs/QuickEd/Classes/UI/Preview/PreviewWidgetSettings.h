#pragma once

#include <TArc/DataProcessing/SettingsNode.h>

#include <Base/BaseTypes.h>
#include <Math/Color.h>
#include <Reflection/Reflection.h>

class PreviewWidgetSettings : public DAVA::SettingsNode
{
public:
    static const DAVA::Color defaultBackgroundColor0;
    static const DAVA::Color defaultBackgroundColor1;
    static const DAVA::Color defaultBackgroundColor2;

    DAVA::Vector<DAVA::Color> backgroundColors = { defaultBackgroundColor0, defaultBackgroundColor1, defaultBackgroundColor2 };
    DAVA::uint32 backgroundColorIndex = 0;

    DAVA_VIRTUAL_REFLECTION(PreviewWidgetSettings, DAVA::SettingsNode);

private:
    // SettingsNode
    void Load(const DAVA::PropertiesItem& settingsNode) override;
    void Save(DAVA::PropertiesItem& settingsNode) const override;

    void LoadVersion0(const DAVA::PropertiesItem& settingsNode);
    void LoadVersion1(const DAVA::PropertiesItem& settingsNode, DAVA::Reflection& settingsReflection);
    void LoadDefaultValues();

    DAVA::uint32 version = 1;
};