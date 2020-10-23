#ifndef __DAVAENGINE_RECT_PACKER__
#define __DAVAENGINE_RECT_PACKER__

#include "Base/BaseMath.h"

namespace DAVA
{
//! helper class to simplify packing of many small 2D images to one big 2D image
class RectPacker
{
public:
    //! \brief constructor
    //! \param[in] size of this RectPacker
    RectPacker(const Rect2i& _rect);

    //! \brief destructor
    virtual ~RectPacker();

    //! \brief release all data allocated by packer and reset it internal state
    void Release();

    //! \brief Add rect to packer, packer must allocate position for this rect
    //! \param[in] rectSize image size of rect we want to pack
    //! \return true if rect was successfully added, false if not
    bool AddRect(const Size2i& rectSize, void* searchPtr);
    Rect2i* SearchRectForPtr(void* searchPtr);

    Rect2i& GetRect()
    {
        return rect;
    }

private:
    // Implementation details
    Rect2i rect;

    struct PackNode
    {
        PackNode()
        {
            isLeaf = true;
            child[0] = 0;
            child[1] = 0;
            isImageSet = false;
            searchPtr = 0;
        }

        bool isImageSet;
        Rect2i rect;
        bool isLeaf;
        PackNode* child[2];
        void* searchPtr;

        PackNode* Insert(const Size2i& imageSize);
        Rect2i* SearchRectForPtr(void* searchPtr);
        void Release();
    };

    PackNode* root;
};
};

#endif //__DAVAENGINE_RECT_PACKER__
