#include "Math/RectanglePacker/RectanglePacker.h"
#include "Math/RectanglePacker/Spritesheet.h"
#include "Render/Texture.h"
#include "Logger/Logger.h"

namespace DAVA
{
RectanglePacker::RectanglePacker()
{
}

std::unique_ptr<RectanglePacker::PackResult> RectanglePacker::Pack(RectanglePacker::PackTask& packTask) const
{
    DVASSERT(packAlgorithms.empty() == false, "Packing algorithm was not specified");
    Vector<SpriteItem> spritesToPack;
    for (const std::shared_ptr<SpriteDefinition>& defFile : packTask.spriteList)
    {
        uint32 frameCount = defFile->GetFrameCount();
        for (uint32 frame = 0; frame < frameCount; ++frame)
        {
            SpriteItem spriteItem;
            spriteItem.spriteWeight = defFile->GetFrameWidth(frame) * defFile->GetFrameHeight(frame);
            spriteItem.defFile = defFile;
            spriteItem.frameIndex = frame;
            spritesToPack.push_back(spriteItem);
        }
    }

    std::stable_sort(spritesToPack.begin(), spritesToPack.end(),
                     [](const SpriteItem& a, const SpriteItem& b) { return a.spriteWeight > b.spriteWeight; });

    return PackSprites(spritesToPack, packTask);
}

std::unique_ptr<RectanglePacker::PackResult> RectanglePacker::PackSprites(Vector<RectanglePacker::SpriteItem>& spritesToPack, RectanglePacker::PackTask& packTask) const
{
    auto packResult = std::make_unique<PackResult>();

    while (false == spritesToPack.empty())
    {
        Logger::FrameworkDebug("* Packing attempts started: ");

        std::unique_ptr<SpritesheetLayout> bestSheet;
        uint32 bestSpritesWeight = 0;
        uint32 bestSheetWeight = 0;
        bool wasFullyPacked = false;
        Vector<SpriteItem> bestSpritesRemaining;

        bool needOnlySquareTexture = onlySquareTextures || packTask.needSquareTextureOverriden;
        for (uint32 yResolution = Texture::MINIMAL_HEIGHT; yResolution <= maxTextureSize; yResolution *= 2)
        {
            for (uint32 xResolution = Texture::MINIMAL_WIDTH; xResolution <= maxTextureSize; xResolution *= 2)
            {
                if (needOnlySquareTexture && (xResolution != yResolution))
                    continue;

                uint32 sheetWeight = xResolution * yResolution;

                if (wasFullyPacked && sheetWeight >= bestSheetWeight)
                    continue;

                for (const PackingAlgorithm alg : packAlgorithms)
                {
                    std::unique_ptr<SpritesheetLayout> sheet = SpritesheetLayout::Create(xResolution, yResolution, useTwoSideMargin, texturesMargin, alg);

                    Vector<SpriteItem> tempSpritesRemaining = spritesToPack;
                    uint32 spritesWeight = TryToPack(sheet.get(), tempSpritesRemaining, wasFullyPacked);

                    bool nowFullyPacked = tempSpritesRemaining.empty();

                    if (wasFullyPacked && !nowFullyPacked)
                        continue;

                    if (nowFullyPacked || spritesWeight > bestSpritesWeight || (spritesWeight == bestSpritesWeight && sheetWeight < bestSheetWeight))
                    {
                        bestSpritesWeight = spritesWeight;
                        bestSheetWeight = sheetWeight;
                        bestSheet = std::move(sheet);
                        bestSpritesRemaining.swap(tempSpritesRemaining);

                        if (nowFullyPacked)
                        {
                            wasFullyPacked = true;
                            break;
                        }
                    }
                }
            }
        }

        if (bestSpritesWeight == 0)
        {
            packResult->resultErrors.insert("Can't pack any sprite. Probably maxTextureSize should be altered");
            break;
        }

        spritesToPack.swap(bestSpritesRemaining);
        packResult->resultSheets.emplace_back(std::move(bestSheet));
    }
    if (packResult->Success())
    {
        CreateSpritesIndex(packTask, packResult.get());
    }
    return packResult;
}

uint32 RectanglePacker::TryToPack(SpritesheetLayout* sheet, Vector<SpriteItem>& tempSortVector, bool fullPackOnly) const
{
    uint32 weight = 0;

    for (uint32 i = 0; i < tempSortVector.size();)
    {
        const std::shared_ptr<SpriteDefinition>& defFile = tempSortVector[i].defFile;
        uint32 frame = tempSortVector[i].frameIndex;
        if (sheet->AddSprite(defFile->GetFrameSize(frame), &defFile->frameRects[frame]))
        {
            weight += tempSortVector[i].spriteWeight;
            tempSortVector.erase(tempSortVector.begin() + i);
        }
        else if (fullPackOnly)
        {
            return weight;
        }
        else
        {
            ++i;
        }
    }
    return weight;
}

void RectanglePacker::CreateSpritesIndex(RectanglePacker::PackTask& packTask, RectanglePacker::PackResult* packResult) const
{
    packResult->resultIndexedSprites.resize(packTask.spriteList.size());

    for (uint32 spriteIndex = 0; spriteIndex < packTask.spriteList.size(); spriteIndex++)
    {
        const std::shared_ptr<SpriteDefinition>& spriteDef = packTask.spriteList[spriteIndex];
        uint32 frameCount = spriteDef->GetFrameCount();

        RectanglePacker::SpriteIndexedData& spriteIndexedData = packResult->resultIndexedSprites[spriteIndex];
        spriteIndexedData.spriteDef = spriteDef;
        spriteIndexedData.frameToSheetIndex.resize(frameCount, -1);
        spriteIndexedData.frameToPackedInfo.resize(frameCount, nullptr);

        for (uint32 frame = 0; frame < frameCount; ++frame)
        {
            const SpriteBoundsRect* packedInfo = nullptr;
            int32 resultSheetsSize = static_cast<int32>(packResult->resultSheets.size());
            for (int32 sheetIndex = 0; sheetIndex < resultSheetsSize; ++sheetIndex)
            {
                packedInfo = packResult->resultSheets[sheetIndex]->GetSpriteBoundsRect(&spriteDef->frameRects[frame]);
                if (packedInfo)
                {
                    spriteIndexedData.frameToSheetIndex[frame] = sheetIndex;
                    spriteIndexedData.frameToPackedInfo[frame] = packedInfo;
                    break;
                }
            }
            DVASSERT(packedInfo != nullptr);
        }
    }
    DVASSERT(packResult->resultIndexedSprites.size() == packTask.spriteList.size());
}
}
