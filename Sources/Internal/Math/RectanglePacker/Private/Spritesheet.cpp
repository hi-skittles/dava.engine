#include "Math/RectanglePacker/Spritesheet.h"

#include "Base/BaseTypes.h"
#include "Utils/StringFormat.h"
#include "Logger/Logger.h"
#include "Base/GlobalEnum.h"

ENUM_DECLARE(DAVA::PackingAlgorithm)
{
    ENUM_ADD_DESCR(static_cast<int>(DAVA::PackingAlgorithm::ALG_BASIC), "ALG_BASIC");
    ENUM_ADD_DESCR(static_cast<int>(DAVA::PackingAlgorithm::ALG_MAXRECTS_BOTTOM_LEFT), "ALG_MAXRECTS_BOTTOM_LEFT");
    ENUM_ADD_DESCR(static_cast<int>(DAVA::PackingAlgorithm::ALG_MAXRECTS_BEST_AREA_FIT), "ALG_MAXRECTS_BEST_AREA_FIT");
    ENUM_ADD_DESCR(static_cast<int>(DAVA::PackingAlgorithm::ALG_MAXRECTS_BEST_SHORT_SIDE_FIT), "ALG_MAXRECTS_BEST_SHORT_SIDE_FIT");
    ENUM_ADD_DESCR(static_cast<int>(DAVA::PackingAlgorithm::ALG_MAXRECTS_BEST_LONG_SIDE_FIT), "ALG_MAXRECTS_BEST_LONG_SIDE_FIT");
    ENUM_ADD_DESCR(static_cast<int>(DAVA::PackingAlgorithm::ALG_MAXRRECT_BEST_CONTACT_POINT), "ALG_MAXRRECT_BEST_CONTACT_POINT");
};

namespace DAVA
{
class BasicSpritesheetLayout : public SpritesheetLayout
{
public:
    explicit BasicSpritesheetLayout(uint32 w, uint32 h, bool duplicateEdgePixel, int32 spritesMargin);

    bool AddSprite(const Size2i& spriteSize, const void* searchPtr) override;
    const SpriteBoundsRect* GetSpriteBoundsRect(const void* searchPtr) const override;
    const Rect2i& GetRect() const override
    {
        return rootNode.cell.marginsRect;
    }
    uint32 GetWeight() const override
    {
        return rootNode.cell.marginsRect.dx * rootNode.cell.marginsRect.dy;
    }

private:
    struct SpritesheetNode
    {
        std::unique_ptr<SpritesheetNode> child[2];
        SpriteBoundsRect cell;
        const void* spritePtr = nullptr;
    };

    SpritesheetNode* Insert(SpritesheetNode* node, const Size2i& imageSize, const void* imagePtr);
    const SpritesheetNode* SearchNodeForPtr(const SpritesheetNode* node, const void* imagePtr) const;

    const int32 edgePixel;
    const int32 spritesMargin;
    const int32 splitter;
    SpritesheetNode rootNode;
};

BasicSpritesheetLayout::BasicSpritesheetLayout(uint32 w, uint32 h, bool duplicateEdgePixel, int32 margin)
    : edgePixel(duplicateEdgePixel ? 1 : 0)
    , spritesMargin(margin)
    , splitter(spritesMargin + edgePixel + edgePixel)
{
    rootNode.cell.marginsRect = Rect2i(0, 0, w, h);
}

BasicSpritesheetLayout::SpritesheetNode* BasicSpritesheetLayout::Insert(SpritesheetNode* node, const Size2i& spriteSize, const void* spritePtr)
{
    DVASSERT(node != nullptr);

    if (node->child[0] != nullptr)
    {
        DVASSERT(node->child[1] != nullptr);

        SpritesheetNode* insResult = Insert(node->child[0].get(), spriteSize, spritePtr);
        if (insResult != nullptr)
        {
            return insResult;
        }
        else
        {
            return Insert(node->child[1].get(), spriteSize, spritePtr);
        }
    }
    else
    {
        if (node->spritePtr != nullptr)
        {
            return nullptr;
        }

        SpriteBoundsRect& cell = node->cell;

        int32 occupiedWidth = spriteSize.dx + cell.leftEdgePixel + cell.rightEdgePixel + cell.rightMargin;
        int32 occupiedHeight = spriteSize.dy + cell.topEdgePixel + cell.bottomEdgePixel + cell.bottomMargin;
        int32 restWidth = cell.marginsRect.dx - occupiedWidth;
        int32 restHeight = cell.marginsRect.dy - occupiedHeight;

        auto SetImage = [&] {
            node->spritePtr = spritePtr;
            cell.spriteRect.dx = spriteSize.dx;
            cell.spriteRect.dy = spriteSize.dy;
            cell.spriteRect.x = cell.marginsRect.x + cell.leftEdgePixel;
            cell.spriteRect.y = cell.marginsRect.y + cell.topEdgePixel;
        };

        if (restWidth == 0 && restHeight == 0)
        {
            SetImage();
            return node;
        }
        else if (restWidth < 0 || restHeight < 0)
        {
            return nullptr;
        }
        else
        {
            int32 longest = Max(restWidth, restHeight);
            if (longest <= splitter) // it's no use to make split
            {
                // try to add edge pixel anyway
                if (longest == restWidth && cell.rightEdgePixel == 0 && restWidth >= edgePixel)
                {
                    restWidth -= edgePixel;
                    cell.rightEdgePixel = edgePixel;
                }
                else if (longest == restHeight && cell.bottomEdgePixel == 0 && restHeight >= edgePixel)
                {
                    restHeight -= edgePixel;
                    cell.bottomEdgePixel = edgePixel;
                }

                cell.rightMargin += restWidth;
                cell.bottomMargin += restHeight;
                SetImage();
                return node;
            }

            node->child[0].reset(new SpritesheetNode);
            node->child[1].reset(new SpritesheetNode);

            SpriteBoundsRect& cell0 = node->child[0]->cell;
            SpriteBoundsRect& cell1 = node->child[1]->cell;

            cell0 = cell1 = cell;

            if (longest == restWidth) // horizontal split
            {
                cell0.rightEdgePixel = edgePixel;
                cell0.rightMargin = spritesMargin;
                cell1.leftEdgePixel = edgePixel;

                int32 cell0width = spriteSize.dx + cell0.leftEdgePixel + cell0.rightEdgePixel + cell0.rightMargin;

                cell0.marginsRect = Rect2i(cell.marginsRect.x, cell.marginsRect.y, cell0width, cell.marginsRect.dy);
                cell1.marginsRect = Rect2i(cell.marginsRect.x + cell0width, cell.marginsRect.y, cell.marginsRect.dx - cell0width, cell.marginsRect.dy);
            }
            else // vertical split
            {
                cell0.bottomEdgePixel = edgePixel;
                cell0.bottomMargin = spritesMargin;
                cell1.topEdgePixel = edgePixel;

                int32 cell0height = spriteSize.dy + cell0.topEdgePixel + cell0.bottomEdgePixel + cell0.bottomMargin;

                cell0.marginsRect = Rect2i(cell.marginsRect.x, cell.marginsRect.y, cell.marginsRect.dx, cell0height);
                cell1.marginsRect = Rect2i(cell.marginsRect.x, cell.marginsRect.y + cell0height, cell.marginsRect.dx, cell.marginsRect.dy - cell0height);
            }

            return Insert(node->child[0].get(), spriteSize, spritePtr);
        }
    }
}

bool BasicSpritesheetLayout::AddSprite(const Size2i& spriteSize, const void* spritePtr)
{
    SpritesheetNode* node = Insert(&rootNode, spriteSize, spritePtr);
    if (node != nullptr)
    {
        Logger::FrameworkDebug("sprite set to (%d, %d), sprite size [%d x %d]", node->cell.marginsRect.dx, node->cell.marginsRect.dy, spriteSize.dx, spriteSize.dy);
        return true;
    }
    else
    {
        return false;
    }
}

const SpriteBoundsRect* BasicSpritesheetLayout::GetSpriteBoundsRect(const void* searchPtr) const
{
    const SpritesheetNode* res = SearchNodeForPtr(&rootNode, searchPtr);
    return (res ? &res->cell : nullptr);
}

const BasicSpritesheetLayout::SpritesheetNode* BasicSpritesheetLayout::SearchNodeForPtr(const SpritesheetNode* node, const void* imagePtr) const
{
    if (imagePtr == node->spritePtr)
    {
        return node;
    }
    else
    {
        const BasicSpritesheetLayout::SpritesheetNode* res = nullptr;
        if (node->child[0] != nullptr)
        {
            res = SearchNodeForPtr(node->child[0].get(), imagePtr);
        }
        if (!res && node->child[1] != nullptr)
        {
            res = SearchNodeForPtr(node->child[1].get(), imagePtr);
        }
        return res;
    }
}

//////////////////////////////////////////////////////////////////////////

class MaxRectsSpritesheetLayout : public SpritesheetLayout
{
public:
    explicit MaxRectsSpritesheetLayout(uint32 w, uint32 h, bool duplicateEdgePixel, int32 spritesMargin);

    // SpritesheetLayout
    bool AddSprite(const Size2i& spriteSize, const void* searchPtr) override;
    const SpriteBoundsRect* GetSpriteBoundsRect(const void* searchPtr) const override;
    const Rect2i& GetRect() const override
    {
        return sheetRect;
    }
    uint32 GetWeight() const override
    {
        return sheetRect.dx * sheetRect.dy;
    }

protected:
    virtual const SpriteBoundsRect* FindBestFreeRect(const Size2i& spriteSize) const = 0;
    const SpriteBoundsRect* InsertNewSpriteRect(const SpriteBoundsRect* foundFreeRect, const Size2i& spriteSize, const void* spritePtr);
    void SplitIntersectedFreeRects(const SpriteBoundsRect* newSpriteRect);
    void RemoveRedundantFreeRects();

    const int32 edgePixel;
    const int32 spritesMargin;
    const int32 splitter;

    Rect2i sheetRect;
    List<SpriteBoundsRect> freeRects;
    UnorderedMap<const void*, SpriteBoundsRect> spriteRects;
};

MaxRectsSpritesheetLayout::MaxRectsSpritesheetLayout(uint32 w, uint32 h, bool duplicateEdgePixel, int32 margin)
    : edgePixel(duplicateEdgePixel ? 1 : 0)
    , spritesMargin(margin)
    , splitter(spritesMargin + edgePixel + edgePixel)
{
    sheetRect = Rect2i(0, 0, w, h);
    SpriteBoundsRect firstRect;
    firstRect.marginsRect = firstRect.spriteRect = sheetRect;
    freeRects.push_back(firstRect);
}

bool MaxRectsSpritesheetLayout::AddSprite(const Size2i& spriteSize, const void* spritePtr)
{
    // maxrects alg in brief:
    // step1: find best free rect
    // step2: insert new sprite rect
    // step3: split free rects intersected by new sprite rect
    // step4: remove redundant free rects

    const SpriteBoundsRect* bestFreeRect = FindBestFreeRect(spriteSize);
    if (bestFreeRect == nullptr)
        return false;

    const SpriteBoundsRect* newSpriteRect = InsertNewSpriteRect(bestFreeRect, spriteSize, spritePtr);
    DVASSERT(newSpriteRect != nullptr);

    SplitIntersectedFreeRects(newSpriteRect);
    RemoveRedundantFreeRects();

    return true;
}

const SpriteBoundsRect* MaxRectsSpritesheetLayout::InsertNewSpriteRect(const SpriteBoundsRect* foundFreeRect, const Size2i& spriteSize, const void* spritePtr)
{
    SpriteBoundsRect boundsRect = *foundFreeRect;
    int32 restWidth = boundsRect.spriteRect.dx - spriteSize.dx;
    int32 restHeight = boundsRect.spriteRect.dy - spriteSize.dy;
    boundsRect.spriteRect.dx = spriteSize.dx;
    boundsRect.spriteRect.dy = spriteSize.dy;

    DVASSERT(restWidth >= 0);
    DVASSERT(restHeight >= 0);

    if (restWidth <= splitter)
    {
        if (restWidth >= edgePixel && boundsRect.rightEdgePixel == 0)
        {
            restWidth -= edgePixel;
            boundsRect.rightEdgePixel = edgePixel;
        }
        boundsRect.rightMargin += restWidth;
    }
    else
    {
        boundsRect.rightMargin = spritesMargin;
        boundsRect.rightEdgePixel = edgePixel;
    }

    if (restHeight <= splitter)
    {
        if (restHeight >= edgePixel && boundsRect.bottomEdgePixel == 0)
        {
            restHeight -= edgePixel;
            boundsRect.bottomEdgePixel = edgePixel;
        }
        boundsRect.bottomMargin += restHeight;
    }
    else
    {
        boundsRect.bottomMargin = spritesMargin;
        boundsRect.bottomEdgePixel = edgePixel;
    }

    boundsRect.marginsRect.dx = boundsRect.spriteRect.dx + boundsRect.leftEdgePixel + boundsRect.rightEdgePixel + boundsRect.rightMargin;
    boundsRect.marginsRect.dy = boundsRect.spriteRect.dy + boundsRect.topEdgePixel + boundsRect.bottomEdgePixel + boundsRect.bottomMargin;

    auto insertResult = spriteRects.insert(std::make_pair(spritePtr, boundsRect));
    DVASSERT(insertResult.second == true, "Second attempt to insert same sprite");

    return &(insertResult.first->second);
}

void MaxRectsSpritesheetLayout::SplitIntersectedFreeRects(const SpriteBoundsRect* newSpriteRect)
{
    List<SpriteBoundsRect> newRects;
    for (List<SpriteBoundsRect>::iterator freeRect = freeRects.begin(); freeRect != freeRects.end();)
    {
        const Rect2i& fr = freeRect->marginsRect;
        Rect2i cut = fr.Intersection(newSpriteRect->marginsRect);
        if (cut.dx > 0 && cut.dy > 0)
        {
            int32 freeX0 = fr.x;
            int32 freeX1 = fr.x + fr.dx;
            int32 freeY0 = fr.y;
            int32 freeY1 = fr.y + fr.dy;

            int32 cutX0 = cut.x;
            int32 cutX1 = cut.x + cut.dx;
            int32 cutY0 = cut.y;
            int32 cutY1 = cut.y + cut.dy;

            if (freeY0 < cutY0 && cutY0 < freeY1)
            {
                SpriteBoundsRect newRect = *freeRect;
                newRect.bottomEdgePixel = edgePixel;
                newRect.bottomMargin = spritesMargin;
                newRect.marginsRect.dy = cutY0 - freeY0;
                newRect.spriteRect.dy = newRect.marginsRect.dy - newRect.topEdgePixel - newRect.bottomEdgePixel - newRect.bottomMargin;
                if (newRect.spriteRect.dy > 0)
                {
                    newRects.emplace_back(newRect);
                }
            }

            if (freeY0 < cutY1 && cutY1 < freeY1)
            {
                SpriteBoundsRect newRect = *freeRect;
                newRect.topEdgePixel = edgePixel;
                newRect.marginsRect.y = cutY1;
                newRect.marginsRect.dy = freeY1 - cutY1;
                newRect.spriteRect.y = newRect.marginsRect.y + newRect.topEdgePixel;
                newRect.spriteRect.dy = newRect.marginsRect.dy - newRect.topEdgePixel - newRect.bottomEdgePixel - newRect.bottomMargin;
                if (newRect.spriteRect.dy > 0)
                {
                    newRects.emplace_back(newRect);
                }
            }

            if (freeX0 < cutX0 && cutX0 < freeX1)
            {
                SpriteBoundsRect newRect = *freeRect;
                newRect.rightEdgePixel = edgePixel;
                newRect.rightMargin = spritesMargin;
                newRect.marginsRect.dx = cutX0 - freeX0;
                newRect.spriteRect.dx = newRect.marginsRect.dx - newRect.leftEdgePixel - newRect.rightEdgePixel - newRect.rightMargin;
                if (newRect.spriteRect.dx > 0)
                {
                    newRects.emplace_back(newRect);
                }
            }

            if (freeX0 < cutX1 && cutX1 < freeX1)
            {
                SpriteBoundsRect newRect = *freeRect;
                newRect.leftEdgePixel = edgePixel;
                newRect.marginsRect.x = cutX1;
                newRect.marginsRect.dx = freeX1 - cutX1;
                newRect.spriteRect.x = newRect.marginsRect.x + newRect.leftEdgePixel;
                newRect.spriteRect.dx = newRect.marginsRect.dx - newRect.leftEdgePixel - newRect.rightEdgePixel - newRect.rightMargin;
                if (newRect.spriteRect.dx > 0)
                {
                    newRects.emplace_back(newRect);
                }
            }

            auto delRect = freeRect++;
            freeRects.erase(delRect);
        }
        else
        {
            ++freeRect;
        }
    }

    if (newRects.empty() == false)
    {
        freeRects.splice(freeRects.end(), newRects);
    }
}

void MaxRectsSpritesheetLayout::RemoveRedundantFreeRects()
{
    for (List<SpriteBoundsRect>::iterator freeRect = freeRects.begin(); freeRect != freeRects.end(); ++freeRect)
    {
        for (List<SpriteBoundsRect>::iterator freeRect2 = freeRects.begin(); freeRect2 != freeRects.end();)
        {
            if (freeRect != freeRect2 && freeRect->marginsRect.RectInside(freeRect2->marginsRect))
            {
                auto delRect = freeRect2++;
                freeRects.erase(delRect);
            }
            else
            {
                ++freeRect2;
            }
        }
    }
}

const SpriteBoundsRect* MaxRectsSpritesheetLayout::GetSpriteBoundsRect(const void* searchPtr) const
{
    auto result = spriteRects.find(searchPtr);
    return (result == spriteRects.end() ? nullptr : &(result->second));
}

//////////////////////////////////////////////////////////////////////////

struct MaxRectsSpritesheetLayout_BL : public MaxRectsSpritesheetLayout
{
    MaxRectsSpritesheetLayout_BL(uint32 w, uint32 h, bool dup, int32 margin)
        : MaxRectsSpritesheetLayout(w, h, dup, margin)
    {
    }

protected:
    const SpriteBoundsRect* FindBestFreeRect(const Size2i& spriteSize) const override
    {
        int32 bestY = 0;
        int32 bestX = 0;
        List<SpriteBoundsRect>::const_iterator rectFound = freeRects.cend();

        for (List<SpriteBoundsRect>::const_iterator freeRect = freeRects.cbegin(); freeRect != freeRects.cend(); ++freeRect)
        {
            int32 restWidth = freeRect->spriteRect.dx - spriteSize.dx;
            int32 restHeight = freeRect->spriteRect.dy - spriteSize.dy;
            if (restWidth >= 0 && restHeight >= 0)
            {
                if (rectFound == freeRects.cend() || freeRect->marginsRect.y < bestY || (freeRect->marginsRect.y == bestY && freeRect->marginsRect.x < bestX))
                {
                    rectFound = freeRect;
                    bestY = freeRect->marginsRect.y;
                    bestX = freeRect->marginsRect.x;
                }
            }
        }
        return (rectFound == freeRects.cend() ? nullptr : &(*rectFound));
    }
};

struct MaxRectsSpritesheetLayout_BAF : public MaxRectsSpritesheetLayout
{
    MaxRectsSpritesheetLayout_BAF(uint32 w, uint32 h, bool dup, int32 margin)
        : MaxRectsSpritesheetLayout(w, h, dup, margin)
    {
    }

protected:
    const SpriteBoundsRect* FindBestFreeRect(const Size2i& spriteSize) const override
    {
        int32 bestArea = 0;
        int32 bestShortSide = 0;
        List<SpriteBoundsRect>::const_iterator rectFound = freeRects.cend();

        for (List<SpriteBoundsRect>::const_iterator freeRect = freeRects.cbegin(); freeRect != freeRects.cend(); ++freeRect)
        {
            int32 restWidth = freeRect->spriteRect.dx - spriteSize.dx;
            int32 restHeight = freeRect->spriteRect.dy - spriteSize.dy;
            if (restWidth >= 0 && restHeight >= 0)
            {
                int32 area = freeRect->spriteRect.dx * freeRect->spriteRect.dy;
                int32 shortSide = Min(restWidth, restHeight);

                if (rectFound == freeRects.cend() || area < bestArea || (area == bestArea && shortSide < bestShortSide))
                {
                    rectFound = freeRect;
                    bestArea = area;
                    bestShortSide = shortSide;
                }
            }
        }
        return (rectFound == freeRects.cend() ? nullptr : &(*rectFound));
    }
};

struct MaxRectsSpritesheetLayout_SSF : public MaxRectsSpritesheetLayout
{
    MaxRectsSpritesheetLayout_SSF(uint32 w, uint32 h, bool dup, int32 margin)
        : MaxRectsSpritesheetLayout(w, h, dup, margin)
    {
    }

protected:
    const SpriteBoundsRect* FindBestFreeRect(const Size2i& spriteSize) const override
    {
        int32 bestShortSide = 0;
        List<SpriteBoundsRect>::const_iterator rectFound = freeRects.cend();

        for (List<SpriteBoundsRect>::const_iterator freeRect = freeRects.cbegin(); freeRect != freeRects.cend(); ++freeRect)
        {
            int32 restWidth = freeRect->spriteRect.dx - spriteSize.dx;
            int32 restHeight = freeRect->spriteRect.dy - spriteSize.dy;
            if (restWidth >= 0 && restHeight >= 0)
            {
                int32 shortSide = Min(restWidth, restHeight);
                if (rectFound == freeRects.cend() || shortSide < bestShortSide)
                {
                    rectFound = freeRect;
                    bestShortSide = shortSide;
                }
            }
        }
        return (rectFound == freeRects.cend() ? nullptr : &(*rectFound));
    }
};

struct MaxRectsSpritesheetLayout_LSF : public MaxRectsSpritesheetLayout
{
    MaxRectsSpritesheetLayout_LSF(uint32 w, uint32 h, bool dup, int32 margin)
        : MaxRectsSpritesheetLayout(w, h, dup, margin)
    {
    }

protected:
    const SpriteBoundsRect* FindBestFreeRect(const Size2i& spriteSize) const override
    {
        int32 bestLongSide = 0;
        List<SpriteBoundsRect>::const_iterator rectFound = freeRects.cend();

        for (List<SpriteBoundsRect>::const_iterator freeRect = freeRects.cbegin(); freeRect != freeRects.cend(); ++freeRect)
        {
            int32 restWidth = freeRect->spriteRect.dx - spriteSize.dx;
            int32 restHeight = freeRect->spriteRect.dy - spriteSize.dy;
            if (restWidth >= 0 && restHeight >= 0)
            {
                int32 longSide = Max(restWidth, restHeight);
                if (rectFound == freeRects.cend() || longSide < bestLongSide)
                {
                    rectFound = freeRect;
                    bestLongSide = longSide;
                }
            }
        }
        return (rectFound == freeRects.cend() ? nullptr : &(*rectFound));
    }
};

struct MaxRectsSpritesheetLayout_CP : public MaxRectsSpritesheetLayout
{
    MaxRectsSpritesheetLayout_CP(uint32 w, uint32 h, bool dup, int32 margin)
        : MaxRectsSpritesheetLayout(w, h, dup, margin)
    {
    }

protected:
    const SpriteBoundsRect* FindBestFreeRect(const Size2i& spriteSize) const override
    {
        int32 bestContactPoint = 0;
        int32 bestShortSide = 0;
        List<SpriteBoundsRect>::const_iterator rectFound = freeRects.cend();

        for (List<SpriteBoundsRect>::const_iterator freeRect = freeRects.cbegin(); freeRect != freeRects.cend(); ++freeRect)
        {
            int32 restWidth = freeRect->spriteRect.dx - spriteSize.dx;
            int32 restHeight = freeRect->spriteRect.dy - spriteSize.dy;
            if (restWidth >= 0 && restHeight >= 0)
            {
                int32 contactPoint = freeRect->spriteRect.dx + freeRect->spriteRect.dy;
                if (restWidth == 0)
                    contactPoint += freeRect->spriteRect.dx;
                if (restHeight == 0)
                    contactPoint += freeRect->spriteRect.dy;
                int32 shortSide = Min(restWidth, restHeight);

                if (rectFound == freeRects.cend() || contactPoint < bestContactPoint || (contactPoint == bestContactPoint && shortSide < bestShortSide))
                {
                    rectFound = freeRect;
                    bestContactPoint = contactPoint;
                    bestShortSide = shortSide;
                }
            }
        }
        return (rectFound == freeRects.cend() ? nullptr : &(*rectFound));
    }
};

//////////////////////////////////////////////////////////////////////////

std::unique_ptr<SpritesheetLayout> SpritesheetLayout::Create(uint32 w, uint32 h, bool duplicateEdgePixel, uint32 spritesMargin, PackingAlgorithm alg)
{
    switch (alg)
    {
    case PackingAlgorithm::ALG_BASIC:
        return std::unique_ptr<SpritesheetLayout>(new BasicSpritesheetLayout(w, h, duplicateEdgePixel, spritesMargin));
    case PackingAlgorithm::ALG_MAXRECTS_BOTTOM_LEFT:
        return std::unique_ptr<SpritesheetLayout>(new MaxRectsSpritesheetLayout_BL(w, h, duplicateEdgePixel, spritesMargin));
    case PackingAlgorithm::ALG_MAXRECTS_BEST_AREA_FIT:
        return std::unique_ptr<SpritesheetLayout>(new MaxRectsSpritesheetLayout_BAF(w, h, duplicateEdgePixel, spritesMargin));
    case PackingAlgorithm::ALG_MAXRECTS_BEST_SHORT_SIDE_FIT:
        return std::unique_ptr<SpritesheetLayout>(new MaxRectsSpritesheetLayout_SSF(w, h, duplicateEdgePixel, spritesMargin));
    case PackingAlgorithm::ALG_MAXRECTS_BEST_LONG_SIDE_FIT:
        return std::unique_ptr<SpritesheetLayout>(new MaxRectsSpritesheetLayout_LSF(w, h, duplicateEdgePixel, spritesMargin));
    case PackingAlgorithm::ALG_MAXRRECT_BEST_CONTACT_POINT:
        return std::unique_ptr<SpritesheetLayout>(new MaxRectsSpritesheetLayout_CP(w, h, duplicateEdgePixel, spritesMargin));
    default:
        DVASSERT(false, Format("Unknown algorithm id: %d", alg).c_str());
        return nullptr;
    }
}
};
