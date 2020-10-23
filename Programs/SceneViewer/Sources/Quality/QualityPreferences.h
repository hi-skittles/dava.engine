#pragma once

#include "SceneViewerApp.h"

namespace QualityPreferences
{
/** loads quality preferences from application settings and sets them to QualitySettingsSystem */
void LoadFromSettings(Settings&);

/** gets quality preferences from QualitySettingsSystem and saves them to application settings*/
void SaveToSettings(Settings&);
};
