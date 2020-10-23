#include "Classes/EditorSystems/ControlTransformationSettings.h"

DAVA_VIRTUAL_REFLECTION_IMPL(ControlTransformationSettings)
{
    DAVA::ReflectionRegistrator<ControlTransformationSettings>::Begin()[DAVA::M::DisplayName("Control Transformations"), DAVA::M::SettingsSortKey(90)]
    .ConstructorByPointer()
    .Field("moveMagnetRange", &ControlTransformationSettings::moveMagnetRange)[DAVA::M::DisplayName("Mouse magnet distance on move")]
    .Field("resizeMagnetRange", &ControlTransformationSettings::resizeMagnetRange)[DAVA::M::DisplayName("Mouse magnet distance on resize"), DAVA::M::HiddenField()]
    .Field("pivotMagnetRange", &ControlTransformationSettings::pivotMagnetRange)[DAVA::M::DisplayName("Mouse magnet distance on move pivot point"), DAVA::M::HiddenField()]
    .Field("moveStepByKeyboard2", &ControlTransformationSettings::moveStepByKeyboard2)[DAVA::M::DisplayName("Move distance by keyboard")]
    .Field("expandedmoveStepByKeyboard2", &ControlTransformationSettings::expandedmoveStepByKeyboard2)[DAVA::M::DisplayName("Move distance by keyboard alternate")]
    .Field("shareOfSizeToMagnetPivot", &ControlTransformationSettings::shareOfSizeToMagnetPivot)[DAVA::M::DisplayName("Pivot magnet share")]
    .Field("angleSegment", &ControlTransformationSettings::angleSegment)[DAVA::M::DisplayName("Rotate section angle")]
    .Field("shiftInverted", &ControlTransformationSettings::shiftInverted)[DAVA::M::DisplayName("Invert shift button")]
    .Field("canMagnet", &ControlTransformationSettings::canMagnet)[DAVA::M::DisplayName("Magnet enabled")]
    .Field("showPivot", &ControlTransformationSettings::showPivot)[DAVA::M::DisplayName("Can transform pivot")]
    .Field("showRotate", &ControlTransformationSettings::showRotate)[DAVA::M::DisplayName("Can rotate control")]
    .Field("minimumSelectionRectSize", &ControlTransformationSettings::minimumSelectionRectSize)[DAVA::M::DisplayName("Minimum size of selection rect"), DAVA::M::HiddenField()]
    .End();
}
