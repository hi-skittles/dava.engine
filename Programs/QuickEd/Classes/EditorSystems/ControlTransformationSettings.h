#pragma once

#include <TArc/DataProcessing/SettingsNode.h>

#include <Math/Vector.h>
#include <Base/BaseTypes.h>

class ControlTransformationSettings : public DAVA::SettingsNode
{
public:
    DAVA::Vector2 moveMagnetRange = DAVA::Vector2(7.0f, 7.0f);
    DAVA::Vector2 resizeMagnetRange = DAVA::Vector2(7.0f, 7.0f);
    DAVA::Vector2 pivotMagnetRange = DAVA::Vector2(7.0f, 7.0f);
    DAVA::Vector2 moveStepByKeyboard2 = DAVA::Vector2(10.0f, 10.0f);
    DAVA::Vector2 expandedmoveStepByKeyboard2 = DAVA::Vector2(1.0f, 1.0f);

    DAVA::Vector2 shareOfSizeToMagnetPivot = DAVA::Vector2(0.25f, 0.25f);
    DAVA::float32 angleSegment = 15.0f;
    bool shiftInverted = false;
    bool canMagnet = true;
    bool showPivot = false;
    bool showRotate = false;
    DAVA::Vector2 minimumSelectionRectSize = DAVA::Vector2(5.0f, 5.0f);

    DAVA_VIRTUAL_REFLECTION(ControlTransformationSettings, DAVA::SettingsNode);
};
