#include "Render/RenderOptions.h"

namespace DAVA
{
FastName optionsNames[RenderOptions::OPTIONS_COUNT] =
{
  FastName("Test Option"),

  FastName("Draw Landscape"),
  FastName("Draw Water Refl/Refr"),
  FastName("Draw Opaque Layer"),
  FastName("Draw Transparent Layer"),
  FastName("Draw Sprites"),
  FastName("Draw Shadow Volumes"),
  FastName("Draw Vegetation"),

  FastName("Enable Fog"),

  FastName("Update LODs"),
  FastName("Update Landscape LODs"),
  FastName("Update Animations"),
  FastName("Process Clipping"),
  FastName("Update UI System"),

  FastName("SpeedTree Animations"),
  FastName("Waves System Process"),

  FastName("All Render Enabled"),
  FastName("Texture Loading"),

  FastName("Static Occlusion"),
  FastName("Debug Draw Occlusion"),
  FastName("Enable Visibility System"),

  FastName("Update Particle Emitters"),
  FastName("Draw Particles"),
  FastName("Particle Prepare Buffers"),
  FastName("Albedo mipmaps"),
  FastName("Lightmap mipmaps"),
#if defined(LOCALIZATION_DEBUG)
  FastName("Localization Warnings"),
  FastName("Localization Errors"),
  FastName("Line Break Errors"),
#endif
  FastName("Draw Nondef Glyph"),
  FastName("Highlight Hard Controls"),
  FastName("Debug Draw Rich Items"),
  FastName("Debug Draw Particles")
};

RenderOptions::RenderOptions()
{
    for (int32 i = 0; i < OPTIONS_COUNT; ++i)
    {
        options[i] = true;
    }

    options[DEBUG_DRAW_STATIC_OCCLUSION] = false;
    options[DEBUG_ENABLE_VISIBILITY_SYSTEM] = false;
    options[REPLACE_ALBEDO_MIPMAPS] = false;
    options[REPLACE_LIGHTMAP_MIPMAPS] = false;
#if defined(LOCALIZATION_DEBUG)
    options[DRAW_LOCALIZATION_ERRORS] = false;
    options[DRAW_LOCALIZATION_WARINGS] = false;
    options[DRAW_LINEBREAK_ERRORS] = false;
#endif
    options[DRAW_NONDEF_GLYPH] = false;
    options[HIGHLIGHT_HARD_CONTROLS] = false;
    options[DEBUG_DRAW_RICH_ITEMS] = false;

    options[DEBUG_DRAW_PARTICLES] = false;
}

bool RenderOptions::IsOptionEnabled(RenderOption option)
{
    return options[option];
}

void RenderOptions::SetOption(RenderOption option, bool value)
{
    options[option] = value;
    NotifyObservers();
}

FastName RenderOptions::GetOptionName(RenderOption option)
{
    return optionsNames[option];
}
};
