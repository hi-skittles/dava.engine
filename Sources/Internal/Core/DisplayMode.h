#ifndef __DAVAENGINE_DISPLAYMODE_H__
#define __DAVAENGINE_DISPLAYMODE_H__

namespace DAVA
{
/**
	\ingroup core
	\brief Class that describe display mode supported by device
*/
class DisplayMode
{
public:
    static const int32 DEFAULT_WIDTH = 800;
    static const int32 DEFAULT_HEIGHT = 600;
    static const int32 DEFAULT_BITS_PER_PIXEL = 16;
    static const int32 DEFAULT_DISPLAYFREQUENCY = 0;

public:
    DisplayMode() = default;

    DisplayMode(int32 width_, int32 height_, int32 bpp_, int32 refreshRate_)
        : width(width_)
        , height(height_)
        , bpp(bpp_)
        , refreshRate(refreshRate_)
    {
    }

    bool IsValid()
    {
        return (width > 0 && height > 0 && refreshRate != -1);
    }

    int32 width = 0; //! width of the display mode
    int32 height = 0; //! height of the display mode
    int32 bpp = 0; //! bits per pixel
    int32 refreshRate = -1; //! refresh rate of the display mode, 0 if default
};
};

#endif // __DAVAENGINE_DISPLAYMODE_H__
