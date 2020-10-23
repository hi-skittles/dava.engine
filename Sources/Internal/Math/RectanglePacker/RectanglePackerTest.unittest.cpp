#include "UnitTests/UnitTests.h"

#include "Base/FastName.h"
#include "Base/RefPtr.h"
#include "Concurrency/Thread.h"
#include "Engine/EngineContext.h"
#include "Job/JobManager.h"
#include "Render/2D/Systems/DynamicAtlasSystem.h"
#include "Render/Texture.h"
#include "Render/TextureDescriptor.h"
#include "Render/2D/Sprite.h"
#include "Reflection/Reflection.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIPackage.h"
#include "UI/UIPackageLoader.h"
#include "UI/UIScreen.h"
#include "Utils/StringUtils.h"

using namespace DAVA;

DAVA_TESTCLASS (RectanglePackerTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("RectanglePacker.cpp")
    DECLARE_COVERED_FILES("Spritesheet.cpp")
    END_FILES_COVERED_BY_TESTS();

    DAVA_TEST (BasicTest)
    {
        RectanglePacker rectanglePacker;
        rectanglePacker.SetMaxTextureSize(1024);
        rectanglePacker.SetUseOnlySquareTextures(false);
        rectanglePacker.SetTwoSideMargin(false);
        rectanglePacker.SetTexturesMargin(0);
        rectanglePacker.SetAlgorithms({
        // "maxrect"
        PackingAlgorithm::ALG_MAXRECTS_BEST_AREA_FIT,
        PackingAlgorithm::ALG_MAXRECTS_BEST_LONG_SIDE_FIT,
        PackingAlgorithm::ALG_MAXRECTS_BEST_SHORT_SIDE_FIT,
        PackingAlgorithm::ALG_MAXRECTS_BOTTOM_LEFT,
        PackingAlgorithm::ALG_MAXRRECT_BEST_CONTACT_POINT,
        // "basic"
        PackingAlgorithm::ALG_BASIC
        });

        RectanglePacker::PackTask packTask;
        Vector<DAVA::int32> testData(9);

        for (uint32 i = 0; i < testData.size(); i++)
        {
            testData[i] = i;
            auto spriteDef = std::make_shared<RectanglePacker::SpriteDefinition>();
            Rect2i frameRect(0, 0, 512, 512);
            spriteDef->frameRects.push_back(frameRect);
            spriteDef->frameRects.push_back(frameRect);
            spriteDef->dataPtr = &testData[i];
            packTask.spriteList.push_back(spriteDef);
        }

        auto packResult = rectanglePacker.Pack(packTask);
        TEST_VERIFY(packResult->Success());
        TEST_VERIFY(packResult->resultSheets.size() == 5);
        TEST_VERIFY(packResult->resultErrors.size() == 0);

        for (uint32 i = 0; i < testData.size(); i++)
        {
            const RectanglePacker::SpriteIndexedData& spriteData = packResult->resultIndexedSprites[i];
            TEST_VERIFY(spriteData.spriteDef->dataPtr == &testData[i]);
            uint32 frameCount = static_cast<uint32>(spriteData.frameToSheetIndex.size());
            for (uint32 frame = 0; frame < frameCount; ++frame)
            {
                int32 sheetIndex = spriteData.frameToSheetIndex[frame];
                TEST_VERIFY((-1 < sheetIndex) && (sheetIndex < static_cast<int32>(packResult->resultSheets.size())));
                TEST_VERIFY(spriteData.frameToPackedInfo[frame] != nullptr);
            }
        }
    }

    DAVA_TEST (MarginTest)
    {
        RectanglePacker rectanglePacker;
        rectanglePacker.SetMaxTextureSize(1024);
        rectanglePacker.SetUseOnlySquareTextures(false);
        rectanglePacker.SetTwoSideMargin(false);
        rectanglePacker.SetTexturesMargin(2);
        rectanglePacker.SetAlgorithms({
        // "maxrect_fast"
        PackingAlgorithm::ALG_MAXRECTS_BEST_AREA_FIT
        });

        RectanglePacker::PackTask packTask;
        Vector<DAVA::int32> testData(9);

        for (uint32 i = 0; i < testData.size(); i++)
        {
            testData[i] = i;
            auto spriteDef = std::make_shared<RectanglePacker::SpriteDefinition>();
            Rect2i frameRect(0, 0, 512, 512);
            spriteDef->frameRects.push_back(frameRect);
            spriteDef->dataPtr = &testData[i];
            packTask.spriteList.push_back(spriteDef);
        }
        auto packResult = rectanglePacker.Pack(packTask);
        TEST_VERIFY(packResult->Success());
        TEST_VERIFY(packResult->resultSheets.size() == 9);
        TEST_VERIFY(packResult->resultErrors.size() == 0);
    }

    DAVA_TEST (FailTest)
    {
        RectanglePacker rectanglePacker;
        rectanglePacker.SetMaxTextureSize(128);
        rectanglePacker.SetUseOnlySquareTextures(false);
        rectanglePacker.SetTwoSideMargin(false);
        rectanglePacker.SetTexturesMargin(0);
        rectanglePacker.SetAlgorithms({
        // "maxrect_fast"
        PackingAlgorithm::ALG_MAXRECTS_BEST_AREA_FIT
        });

        RectanglePacker::PackTask packTask;

        auto spriteDef1 = std::make_shared<RectanglePacker::SpriteDefinition>();
        spriteDef1->frameRects.push_back(Rect2i(0, 0, 64, 64));
        packTask.spriteList.push_back(spriteDef1);

        auto spriteDef2 = std::make_shared<RectanglePacker::SpriteDefinition>();
        spriteDef2->frameRects.push_back(Rect2i(0, 0, 512, 512));
        packTask.spriteList.push_back(spriteDef2);

        auto packResult = rectanglePacker.Pack(packTask);
        TEST_VERIFY(packResult->Success() == false);
        TEST_VERIFY(packResult->resultSheets.size() == 1);
        TEST_VERIFY(packResult->resultErrors.size() == 1);
    }
};
