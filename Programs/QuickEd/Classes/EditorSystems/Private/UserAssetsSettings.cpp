#include "Classes/EditorSystems/UserAssetsSettings.h"

#include <Reflection/ReflectionRegistrator.h>

DAVA_VIRTUAL_REFLECTION_IMPL(UserAssetsSettings)
{
    DAVA::ReflectionRegistrator<UserAssetsSettings>::Begin()[DAVA::M::DisplayName("User graphic"), DAVA::M::SettingsSortKey(10)]
    .ConstructorByPointer()
    .Field("selectionRectColor", &UserAssetsSettings::selectionRectColor)[DAVA::M::DisplayName("Selection rect color")]
    .Field("highlightColor", &UserAssetsSettings::highlightColor)[DAVA::M::DisplayName("Highlight color")]
    .Field("hudRectColor", &UserAssetsSettings::hudRectColor)[DAVA::M::DisplayName("Frame rect color")]
    .Field("cornerRectPath2", &UserAssetsSettings::cornerRectPath2)[DAVA::M::DisplayName("Corner rect path")]
    .Field("borderRectPath2", &UserAssetsSettings::borderRectPath2)[DAVA::M::DisplayName("Border rect path")]
    .Field("pivotPointPath2", &UserAssetsSettings::pivotPointPath2)[DAVA::M::DisplayName("Pivot point control path")]
    .Field("rotatePath2", &UserAssetsSettings::rotatePath2)[DAVA::M::DisplayName("Rotate control path")]
    .Field("magnetLinePath2", &UserAssetsSettings::magnetLinePath2)[DAVA::M::DisplayName("Magnet line path")]
    .Field("magnetRectPath2", &UserAssetsSettings::magnetRectPath2)[DAVA::M::DisplayName("Magnet rect path")]
    .Field("guidesColor", &UserAssetsSettings::guidesColor)[DAVA::M::DisplayName("Guide color")]
    .Field("previewGuideColor", &UserAssetsSettings::previewGuideColor)[DAVA::M::DisplayName("Preview guide color")]
    .End();
}
