#if 0

#include "PixelizationController.h"
#include "SpritesHelper.h"

namespace DAVA {

PixelizationController::PixelizationController()
{
}

PixelizationController::~PixelizationController()
{
    Cleanup();
}

void PixelizationController::AddTextBlock(TextBlock* textBlock)
{
    if (textBlock)
    {
        textBlocksList.insert(textBlock);
    }
}

void PixelizationController::SetPixelization(bool value)
{
    for (Set<TextBlock*>::iterator iter = textBlocksList.begin(); iter != textBlocksList.end(); iter ++)
    {
        TextBlock* textBlock = *iter;
        SpritesHelper::SetPixelization(textBlock->GetSprite(), value);
    }
}
    
void PixelizationController::Cleanup()
{
    textBlocksList.clear();
}

};

#endif