#ifndef __PIXELIZATIONCONTROLLER__H__
#define __PIXELIZATIONCONTROLLER__H__

#include "Render/2D/TextBlock.h"

#if 0

namespace DAVA {

// The purpose of this controller is to gather the controls which pixelization settings
// should be updated after the Draw() (currently text controls) and update them accordingly.
class PixelizationController : public Singleton<PixelizationController>
{
public:
    PixelizationController();
    virtual ~PixelizationController();

    // Add the text block to the controls list.
    void AddTextBlock(TextBlock* textBlock);
    
    // Do the pixelization update.
    void SetPixelization(bool value);

protected:
    void Cleanup();

private:
    Set<TextBlock*> textBlocksList;
};

};

#endif

#endif /* defined(__PIXELIZATIONCONTROLLER__H__) */
