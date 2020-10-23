#pragma once

#include <TArc/DataProcessing/SettingsNode.h>
#include <TArc/Qt/QtString.h>
#include <Reflection/Reflection.h>
#include <FileSystem/FilePath.h>
#include <Math/Color.h>

class SlotSystemSettings : public DAVA::TArc::SettingsNode
{
public:
    bool autoGenerateSlotNames = true;
    DAVA::FilePath lastConfigPath;
    QString lastPresetSaveLoadPath;
    DAVA::Color slotBoxColor = DAVA::Color(0.0f, 0.0f, 0.7f, 0.1f);
    DAVA::Color slotBoxEdgesColor = DAVA::Color(0.5f, 0.2f, 0.0f, 1.0f);
    DAVA::float32 pivotPointSize = 0.3f;
    DAVA::Color slotPivotColor = DAVA::Color(0.7f, 0.7f, 0.0f, 0.5f);

    DAVA_VIRTUAL_REFLECTION(SlotSystemSettings, DAVA::TArc::SettingsNode);
};