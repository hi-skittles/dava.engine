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

struct SingleAtlasTester
{
    Vector<RefPtr<Sprite>> mergedSprites;
    Vector<RefPtr<Sprite>> ignoredSprites;

    void MustBeMerged(const FilePath& path)
    {
        mergedSprites.emplace_back(Sprite::Create(path));
    }

    void MustBeIgnored(const FilePath& path)
    {
        ignoredSprites.emplace_back(Sprite::Create(path));
    }

    void Clear()
    {
        mergedSprites.clear();
        ignoredSprites.clear();
    }

    void CheckBeforePack()
    {
        Texture* texture = nullptr;
        for (auto& sprite : mergedSprites)
        {
            TEST_VERIFY(nullptr == sprite->GetTexture(0));
        }
        for (auto& sprite : ignoredSprites)
        {
            TEST_VERIFY(nullptr != sprite->GetTexture(0));
        }
    }

    void CheckAfterPack()
    {
        Texture* texture = nullptr;
        for (auto& sprite : mergedSprites)
        {
            if (texture == nullptr)
            {
                texture = sprite->GetTexture(0);
            }

            TEST_VERIFY(nullptr != sprite->GetTexture(0));
            TEST_VERIFY(texture == sprite->GetTexture(0));
            TEST_VERIFY(StringUtils::StartsWith(texture->texDescriptor->pathname.GetStringValue(), "memoryfile_dynamic_atlas_"));
        }
        Set<Texture*> unique;
        for (auto& sprite : ignoredSprites)
        {
            texture = sprite->GetTexture(0);
            TEST_VERIFY(unique.find(texture) == unique.end());
            unique.insert(texture);
        }
    }
};

DAVA_TESTCLASS (DynamicAtlasSystemTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("DynamicAtlasSystem.cpp")
    END_FILES_COVERED_BY_TESTS();

    const String SPRITE_00 = "~res:/TestData/DynamicAtlasSystemTest/sprite00.tex";
    const String SPRITE_01_WHITE = "~res:/TestData/DynamicAtlasSystemTest/WhiteList/sprite01.txt";
    const String SPRITE_02_WHITE_BLACK = "~res:/TestData/DynamicAtlasSystemTest/WhiteList/BlackList/sprite02.txt";
    const String SPRITE_03_WHITE_BLACK = "~res:/TestData/DynamicAtlasSystemTest/WhiteList/BlackList/sprite03.txt";
    const String SPRITE_04_WHITE_INNER = "~res:/TestData/DynamicAtlasSystemTest/WhiteList/Inner/sprite04.txt";
    const String SPRITE_05_WHITE_INNER = "~res:/TestData/DynamicAtlasSystemTest/WhiteList/Inner/sprite05.txt";
    const String SPRITE_MULTIFRAME = "~res:/TestData/DynamicAtlasSystemTest/WhiteList/Inner/sprite45_multiframe.txt";

    const String SPRITE_LARGE_1024 = "~res:/TestData/DynamicAtlasSystemTest/LargeSprites/large_1024.txt";
    const String SPRITE_LARGE_512_RGBA = "~res:/TestData/DynamicAtlasSystemTest/LargeSprites/large_512_rgba8888.txt";
    const String SPRITE_LARGE_512_RGB = "~res:/TestData/DynamicAtlasSystemTest/LargeSprites/large_512_rgb888.txt";

    const String SPRITE_NO_FILE = "~res:/TestData/DynamicAtlasSystemTest/InvalidSprites/NO_FILE.txt";
    const String SPRITE_NO_IMAGE = "~res:/TestData/DynamicAtlasSystemTest/InvalidSprites/NO_IMAGE.txt";
    const String SPRITE_NO_TEXTURE = "~res:/TestData/DynamicAtlasSystemTest/InvalidSprites/NO_TEXTURE.txt";
    const String SPRITE_CUBEMAP_TEXTURE = "~res:/TestData/DynamicAtlasSystemTest/InvalidSprites/CUBEMAP.txt";

    const String WHITE_PATH = "~res:/TestData/DynamicAtlasSystemTest/WhiteList";
    const String BLACK_PATH = "~res:/TestData/DynamicAtlasSystemTest/WhiteList/BlackList";

    DAVA_TEST (ClearTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        dynamicAtlasSystem->BeginAtlas({}, {});

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        RefPtr<Sprite> sprite001(Sprite::Create(SPRITE_LARGE_512_RGBA));
        RefPtr<Sprite> sprite002(Sprite::Create(SPRITE_LARGE_512_RGB));

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        dynamicAtlasSystem->EndAtlas();

        TEST_VERIFY(sprite001->GetTexture(0) == sprite002->GetTexture(0));

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        dynamicAtlasSystem->Clear();

        TEST_VERIFY(sprite001->GetTexture(0) != sprite002->GetTexture(0));
        TEST_VERIFY(sprite001->GetTexture(0) != nullptr);
        TEST_VERIFY(sprite002->GetTexture(0) != nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (MultiframeTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        dynamicAtlasSystem->BeginAtlas({}, {});

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        RefPtr<Sprite> sprite001(Sprite::Create(SPRITE_MULTIFRAME));
        TEST_VERIFY(sprite001->GetFrameCount() == 2);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 1);

        TEST_VERIFY(sprite001->GetTexture(0) == nullptr);
        TEST_VERIFY(sprite001->GetTexture(1) == nullptr);

        dynamicAtlasSystem->EndAtlas();

        TEST_VERIFY(sprite001->GetTexture(0) != nullptr);
        TEST_VERIFY(sprite001->GetTexture(0) == sprite001->GetTexture(1));

        dynamicAtlasSystem->Clear();

        TEST_VERIFY(sprite001->GetTexture(0) != nullptr);
        TEST_VERIFY(sprite001->GetTexture(0) != sprite001->GetTexture(1));
    }

    DAVA_TEST (SpriteReloadTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        dynamicAtlasSystem->BeginAtlas({}, {});

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        RefPtr<Sprite> sprite001(Sprite::Create(SPRITE_LARGE_512_RGBA));
        RefPtr<Sprite> sprite002(Sprite::Create(SPRITE_LARGE_512_RGB));

        TEST_VERIFY(sprite001->GetTexture(0) == nullptr);
        TEST_VERIFY(sprite002->GetTexture(0) == nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        sprite001->Reload();

        TEST_VERIFY(sprite001->GetTexture(0) == nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        dynamicAtlasSystem->EndAtlas();

        TEST_VERIFY(sprite001->GetTexture(0) == sprite002->GetTexture(0));

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        sprite001->Reload();

        TEST_VERIFY(sprite001->GetTexture(0) != sprite002->GetTexture(0));
        TEST_VERIFY(sprite001->GetTexture(0) != nullptr);
        TEST_VERIFY(sprite002->GetTexture(0) != nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 1);

        sprite002->Reload();

        TEST_VERIFY(sprite001->GetTexture(0) != sprite002->GetTexture(0));
        TEST_VERIFY(sprite001->GetTexture(0) != nullptr);
        TEST_VERIFY(sprite002->GetTexture(0) != nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (RepackTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        dynamicAtlasSystem->BeginAtlas({}, {});

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        RefPtr<Sprite> sprite000(Sprite::Create(SPRITE_LARGE_1024));
        RefPtr<Sprite> sprite001(Sprite::Create(SPRITE_LARGE_512_RGBA));
        RefPtr<Sprite> sprite002(Sprite::Create(SPRITE_LARGE_512_RGB));

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 3);

        dynamicAtlasSystem->EndAtlas();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 2);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 3);

        sprite001.Set(nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 2);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        TEST_VERIFY(sprite000->GetTexture(0) != sprite002->GetTexture(0));

        dynamicAtlasSystem->RebuildAll();

        TEST_VERIFY(sprite000->GetTexture(0) == sprite002->GetTexture(0));

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        sprite002.Set(nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 1);

        sprite000.Set(nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (RegistrationTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        dynamicAtlasSystem->BeginAtlas({}, {});

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        RefPtr<Sprite> sprite001(Sprite::Create(SPRITE_01_WHITE));
        RefPtr<Sprite> sprite002(Sprite::Create(SPRITE_02_WHITE_BLACK));
        // Invalid sprites
        RefPtr<Sprite> spriteNoFile(Sprite::Create(SPRITE_NO_FILE));
        RefPtr<Sprite> spriteNoImage(Sprite::Create(SPRITE_NO_IMAGE));
        RefPtr<Sprite> spriteNoTexture(Sprite::Create(SPRITE_NO_TEXTURE));
        RefPtr<Sprite> spriteCubemap(Sprite::Create(SPRITE_CUBEMAP_TEXTURE));

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 3);

        sprite001.Set(nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        dynamicAtlasSystem->EndAtlas();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        sprite002.Set(nullptr);
        spriteNoImage.Set(nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (PackAllTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        dynamicAtlasSystem->BeginAtlas({}, {});

        SingleAtlasTester tester;

        tester.MustBeMerged(SPRITE_01_WHITE);
        tester.MustBeMerged(SPRITE_04_WHITE_INNER);
        tester.MustBeMerged(SPRITE_05_WHITE_INNER);
        tester.MustBeMerged(SPRITE_00);
        tester.MustBeMerged(SPRITE_02_WHITE_BLACK);
        tester.MustBeMerged(SPRITE_03_WHITE_BLACK);

        tester.CheckBeforePack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 6);

        dynamicAtlasSystem->EndAtlas();

        tester.CheckAfterPack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 6);

        RefPtr<Sprite> danglingSprite(Sprite::Create(SPRITE_00));

        tester.Clear();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 1);

        tester.MustBeIgnored(SPRITE_01_WHITE);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 1);

        dynamicAtlasSystem->BeginAtlas({}, {});

        tester.MustBeMerged(SPRITE_02_WHITE_BLACK);
        tester.MustBeMerged(SPRITE_03_WHITE_BLACK);

        tester.CheckBeforePack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 3);

        dynamicAtlasSystem->EndAtlas();

        tester.CheckAfterPack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 3);

        TEST_VERIFY(danglingSprite->GetTexture(0) == tester.mergedSprites[0]->GetTexture(0));

        danglingSprite.Set(nullptr);

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        tester.Clear();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (DoublePackTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        dynamicAtlasSystem->BeginAtlas({}, {});

        RefPtr<Sprite> sprite001(Sprite::Create(SPRITE_01_WHITE));
        RefPtr<Sprite> sprite004(Sprite::Create(SPRITE_04_WHITE_INNER));

        dynamicAtlasSystem->EndAtlas();

        dynamicAtlasSystem->BeginAtlas({}, {});

        RefPtr<Sprite> sprite005(Sprite::Create(SPRITE_05_WHITE_INNER));
        RefPtr<Sprite> sprite000(Sprite::Create(SPRITE_00));

        dynamicAtlasSystem->EndAtlas();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 2);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 4);

        sprite001 = nullptr;
        sprite005 = nullptr;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 2);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 2);

        sprite000 = nullptr;
        sprite004 = nullptr;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (WhiteListAndBlackListTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        dynamicAtlasSystem->BeginAtlas({ WHITE_PATH }, { BLACK_PATH });

        SingleAtlasTester tester;

        tester.MustBeMerged(SPRITE_01_WHITE);
        tester.MustBeMerged(SPRITE_04_WHITE_INNER);
        tester.MustBeMerged(SPRITE_05_WHITE_INNER);

        tester.MustBeIgnored(SPRITE_00);
        tester.MustBeIgnored(SPRITE_02_WHITE_BLACK);
        tester.MustBeIgnored(SPRITE_03_WHITE_BLACK);

        tester.CheckBeforePack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 3);

        dynamicAtlasSystem->EndAtlas();

        tester.CheckAfterPack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 3);

        tester.Clear();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (WhiteListTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        dynamicAtlasSystem->BeginAtlas({ WHITE_PATH }, {});

        SingleAtlasTester tester;

        tester.MustBeMerged(SPRITE_01_WHITE);
        tester.MustBeMerged(SPRITE_04_WHITE_INNER);
        tester.MustBeMerged(SPRITE_05_WHITE_INNER);
        tester.MustBeMerged(SPRITE_02_WHITE_BLACK);
        tester.MustBeMerged(SPRITE_03_WHITE_BLACK);

        tester.MustBeIgnored(SPRITE_00);

        tester.CheckBeforePack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 5);

        dynamicAtlasSystem->EndAtlas();

        tester.CheckAfterPack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 5);

        tester.Clear();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (BlackListTest)
    {
        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        dynamicAtlasSystem->BeginAtlas({}, { BLACK_PATH });

        SingleAtlasTester tester;

        tester.MustBeMerged(SPRITE_00);
        tester.MustBeMerged(SPRITE_01_WHITE);
        tester.MustBeMerged(SPRITE_04_WHITE_INNER);
        tester.MustBeMerged(SPRITE_05_WHITE_INNER);

        tester.MustBeIgnored(SPRITE_02_WHITE_BLACK);
        tester.MustBeIgnored(SPRITE_03_WHITE_BLACK);

        tester.CheckBeforePack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 4);

        dynamicAtlasSystem->EndAtlas();

        tester.CheckAfterPack();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 1);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 4);

        tester.Clear();

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }

    DAVA_TEST (LeaksTest)
    {
        auto globalTexturesCount = Texture::GetTextureMap().size();

        DynamicAtlasSystem* dynamicAtlasSystem = GetEngineContext()->dynamicAtlasSystem;

        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);

        dynamicAtlasSystem->BeginAtlas({ WHITE_PATH }, { BLACK_PATH });

        SingleAtlasTester tester;

        tester.MustBeMerged(SPRITE_01_WHITE);
        tester.MustBeMerged(SPRITE_04_WHITE_INNER);
        tester.MustBeMerged(SPRITE_05_WHITE_INNER);

        tester.MustBeIgnored(SPRITE_00);
        tester.MustBeIgnored(SPRITE_02_WHITE_BLACK);

        tester.CheckBeforePack();

        TEST_VERIFY(Texture::GetTextureMap().size() - globalTexturesCount == 2);

        dynamicAtlasSystem->EndAtlas();

        tester.CheckAfterPack();

        TEST_VERIFY(Texture::GetTextureMap().size() - globalTexturesCount == 2);

        tester.Clear();

        TEST_VERIFY(Texture::GetTextureMap().size() - globalTexturesCount == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetAtlasesCount() == 0);
        TEST_VERIFY(dynamicAtlasSystem->GetRegionsCount() == 0);
    }
};
