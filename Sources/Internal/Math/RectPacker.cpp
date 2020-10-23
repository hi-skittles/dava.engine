#include "Math/RectPacker.h"

namespace DAVA
{
RectPacker::PackNode* RectPacker::PackNode::Insert(const Size2i& imageSize)
{
    if (!isLeaf)
    {
        RectPacker::PackNode* newNode = child[0]->Insert(imageSize);
        if (newNode)
            return newNode;

        return child[1]->Insert(imageSize);
    }
    else
    {
        if (isImageSet)
            return 0;

        if ((imageSize.dx > rect.dx) || (imageSize.dy > rect.dy))
        {
            return 0;
        }
        if ((imageSize.dx == rect.dx) && (imageSize.dy == rect.dy))
        {
            isImageSet = true;
            return this;
        }

        isLeaf = false;

        child[0] = new RectPacker::PackNode;
        child[1] = new RectPacker::PackNode;

        int32 dw = rect.dx - imageSize.dx;
        int32 dh = rect.dy - imageSize.dy;

        if (dw > dh)
        {
            child[0]->rect = Rect2i(rect.x, rect.y, imageSize.dx, rect.dy);
            child[1]->rect = Rect2i(rect.x + imageSize.dx, rect.y, rect.dx - imageSize.dx, rect.dy);
        }
        else
        {
            child[0]->rect = Rect2i(rect.x, rect.y, rect.dx, imageSize.dy);
            child[1]->rect = Rect2i(rect.x, rect.y + imageSize.dy, rect.dx, rect.dy - imageSize.dy);
        }
        return child[0]->Insert(imageSize);
    }
}

void RectPacker::PackNode::Release()
{
    if (child[0])
        child[0]->Release();

    if (child[1])
        child[1]->Release();

    delete this;
}

RectPacker::RectPacker(const Rect2i& _rect)
{
    root = new PackNode;
    root->rect = _rect;
    rect = _rect;
}

RectPacker::~RectPacker()
{
    Release();
}

void RectPacker::Release()
{
    if (root)
    {
        root->Release();
        root = 0;
    }
}

bool RectPacker::AddRect(const Size2i& imageSize, void* searchPtr)
{
    PackNode* node = root->Insert(imageSize);
    if (node)
    {
        node->searchPtr = searchPtr;
    }
    return (node != 0);
}

Rect2i* RectPacker::SearchRectForPtr(void* searchPtr)
{
    return root->SearchRectForPtr(searchPtr);
}

Rect2i* RectPacker::PackNode::SearchRectForPtr(void* searchPtr)
{
    if (searchPtr == this->searchPtr)
    {
        return &this->rect;
    }
    else
    {
        Rect2i* resultRect = 0;
        if (child[0])
            resultRect = child[0]->SearchRectForPtr(searchPtr);
        if (child[1] && (!resultRect))
            resultRect = child[1]->SearchRectForPtr(searchPtr);

        return resultRect;
    }
}
};