#ifndef __SCENE_STATE_H__
#define __SCENE_STATE_H__

#include "Base/BaseTypes.h"

enum ST_Axis
{
    ST_AXIS_NONE = 0,

    ST_AXIS_X = 0x1,
    ST_AXIS_Y = 0x2,
    ST_AXIS_Z = 0x4,
    ST_AXIS_XY = ST_AXIS_X | ST_AXIS_Y,
    ST_AXIS_XZ = ST_AXIS_X | ST_AXIS_Z,
    ST_AXIS_YZ = ST_AXIS_Y | ST_AXIS_Z,
};

enum eEditorMode : DAVA::uint32
{
    MODE_ALL_SCENE = 0,
    MODE_SELECTION,

    MODE_COUNT,
    MODE_DEFAULT = MODE_SELECTION
};


#endif // __SCENE_STATE_H__
