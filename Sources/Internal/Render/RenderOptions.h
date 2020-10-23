#ifndef __DAVAENGINE_RENDEROPTIONS_H__
#define __DAVAENGINE_RENDEROPTIONS_H__

#include "Base/BaseTypes.h"
#include "Base/Observable.h"
#include "Base/FastName.h"

namespace DAVA
{
class RenderOptions : public Observable
{
public:
    enum RenderOption
    {
        TEST_OPTION = 0,

        LANDSCAPE_DRAW,
        WATER_REFLECTION_REFRACTION_DRAW,
        OPAQUE_DRAW,
        TRANSPARENT_DRAW,
        SPRITE_DRAW,
        SHADOWVOLUME_DRAW,
        VEGETATION_DRAW,

        FOG_ENABLE,

        UPDATE_LODS,
        UPDATE_LANDSCAPE_LODS,
        UPDATE_ANIMATIONS,
        PROCESS_CLIPPING,
        UPDATE_UI_CONTROL_SYSTEM,

        SPEEDTREE_ANIMATIONS,
        WAVE_DISTURBANCE_PROCESS,

        ALL_RENDER_FUNCTIONS_ENABLED,
        TEXTURE_LOAD_ENABLED,

        ENABLE_STATIC_OCCLUSION,
        DEBUG_DRAW_STATIC_OCCLUSION,
        DEBUG_ENABLE_VISIBILITY_SYSTEM,

        UPDATE_PARTICLE_EMMITERS,
        PARTICLES_DRAW,
        PARTICLES_PREPARE_BUFFERS,
        REPLACE_ALBEDO_MIPMAPS,
        REPLACE_LIGHTMAP_MIPMAPS,
#if defined(LOCALIZATION_DEBUG)
        DRAW_LOCALIZATION_WARINGS,
        DRAW_LOCALIZATION_ERRORS,
        DRAW_LINEBREAK_ERRORS,
#endif
        DRAW_NONDEF_GLYPH,
        HIGHLIGHT_HARD_CONTROLS,
        DEBUG_DRAW_RICH_ITEMS,

        DEBUG_DRAW_PARTICLES,

        OPTIONS_COUNT
    };

    bool IsOptionEnabled(RenderOption option);
    void SetOption(RenderOption option, bool value);
    FastName GetOptionName(RenderOption option);
    RenderOptions();

private:
    bool options[OPTIONS_COUNT];
};
};

#endif //__DAVAENGINE_RENDEROPTIONS_H__
