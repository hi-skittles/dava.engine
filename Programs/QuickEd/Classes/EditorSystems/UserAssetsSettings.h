#pragma once

#include <TArc/DataProcessing/SettingsNode.h>
#include <Reflection/Reflection.h>

#include <Math/Color.h>
#include <Base/String.h>

class UserAssetsSettings : public DAVA::SettingsNode
{
public:
    DAVA::Color selectionRectColor = DAVA::Color(0.8f, 0.8f, 0.8f, 0.9f);
    DAVA::Color highlightColor = DAVA::Color(0.26f, 0.75f, 1.0f, 0.9f);
    DAVA::Color hudRectColor = DAVA::Color(0.8f, 0.8f, 0.8f, 0.9f);
    DAVA::String cornerRectPath2 = DAVA::String("~res:/QuickEd/UI/HUDControls/CornerRect.png");
    DAVA::String borderRectPath2 = DAVA::String("~res:/QuickEd/UI/HUDControls/BorderRect.png");
    DAVA::String pivotPointPath2 = DAVA::String("~res:/QuickEd/UI/HUDControls/Pivot.png");
    DAVA::String rotatePath2 = DAVA::String("~res:/QuickEd/UI/HUDControls/Rotate.png");
    DAVA::String magnetLinePath2 = DAVA::String("~res:/QuickEd/UI/HUDControls/MagnetLine/dotline.png");
    DAVA::String magnetRectPath2 = DAVA::String("~res:/QuickEd/UI/HUDControls/MagnetLine/MagnetLine.png");
    DAVA::Color guidesColor = DAVA::Color(1.0f, 0.0f, 0.0f, 1.0f);
    DAVA::Color previewGuideColor = DAVA::Color(1.0f, 0.0f, 0.0f, 0.5f);

    DAVA_VIRTUAL_REFLECTION(UserAssetsSettings, DAVA::SettingsNode);
};
