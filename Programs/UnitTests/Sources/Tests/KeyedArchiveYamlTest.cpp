#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

DAVA_TESTCLASS (KeyedArchiveYamlTest)
{
    DAVA::RefPtr<DAVA::KeyedArchive> loadedArchive;
    DAVA::RefPtr<DAVA::KeyedArchive> testArchive;

    KeyedArchiveYamlTest()
        : loadedArchive(new DAVA::KeyedArchive())
        , testArchive(new DAVA::KeyedArchive())
    {
    }

    DAVA_TEST (TestFunction)
    {
        using namespace DAVA;

        const DAVA::String FILE_PATH("~res:/KeyedArchives/keyed_archive_original.yaml");
        const DAVA::String GENERATED_FILE_PATH("KeyedArchives/keyed_archive_created.yaml");
        //const DAVA::String GENERATED_FILE_PATH "~res:/KeyedArchives/keyed_archive_created.yaml"
        //const DAVA::String GENERATED_FILE_PATH "/Users/user/Documents/work/gitHub/dava.framework/Projects/UnitTests/Data/KeyedArchives/keyed_archive_created.yaml"

        const DAVA::String BOOLMAPID("mapNamebool");
        const DAVA::String INT32MAPID("mapNameint32");
        const DAVA::String UINT32MAPID("mapNameUInt32");
        const DAVA::String FLOATMAPID("mapNamefloat");
        const DAVA::String STRINGMAPID("mapNameString");
        const DAVA::String WSTRINGMAPID("mapNameWideString");
        const DAVA::String BYTEARRMAPID("mapNameByteArrey");
        const DAVA::String INT64MAPID("mapNameint64");
        const DAVA::String UINT64MAPID("mapNameUInt64");
        const DAVA::String VECTOR2MAPID("mapNamevector2");
        const DAVA::String VECTOR3MAPID("mapNameVector3");
        const DAVA::String VECTOR4MAPID("mapNameVector4");
        const DAVA::String MATRIX2MAPID("mapNameMatrix2");
        const DAVA::String MATRIX3MAPID("mapNameMatrix3");
        const DAVA::String MATRIX4MAPID("mapNameMatrix4");
        const DAVA::String KEYEDARCHMAPID("mapNameKArch");
        const DAVA::String INT8MAPID("mapNameInt8");
        const DAVA::String UINT8MAPID("mapNameUInt8");
        const DAVA::String INT16MAPID("mapNameInt16");
        const DAVA::String UINT16MAPID("mapNameUInt16");
        const DAVA::String FLOAT64MAPID("mapNameFloat64");
        const DAVA::String COLORMAPID("mapNameColor");
        const DAVA::String TESTKEY("testKey");

        bool loaded = false;

        loadedArchive->DeleteAllKeys();

        loaded = loadedArchive->LoadFromYamlFile(FILE_PATH);
        TEST_VERIFY(false != loaded);

        FilePath documentsPath = FileSystem::Instance()->GetCurrentDocumentsDirectory();
        FilePath generatedPath = documentsPath + GENERATED_FILE_PATH;

        FileSystem::Instance()->CreateDirectory(generatedPath.GetDirectory(), true);

        loadedArchive->SaveToYamlFile(generatedPath);

        ScopedPtr<KeyedArchive> loadedArchiveFromGeneratedFile(new KeyedArchive());
        loaded = loadedArchiveFromGeneratedFile->LoadFromYamlFile(generatedPath);

        TEST_VERIFY(false != loaded);

        TEST_VERIFY(*loadedArchive->GetVariant(BOOLMAPID) == *loadedArchiveFromGeneratedFile->GetVariant(BOOLMAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(INT32MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(INT32MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(UINT32MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(UINT32MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(FLOATMAPID) == *loadedArchiveFromGeneratedFile->GetVariant(FLOATMAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(STRINGMAPID) == *loadedArchiveFromGeneratedFile->GetVariant(STRINGMAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(WSTRINGMAPID) == *loadedArchiveFromGeneratedFile->GetVariant(WSTRINGMAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(BYTEARRMAPID) == *loadedArchiveFromGeneratedFile->GetVariant(BYTEARRMAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(INT64MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(INT64MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(UINT64MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(UINT64MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(VECTOR2MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(VECTOR2MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(VECTOR3MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(VECTOR3MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(VECTOR4MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(VECTOR4MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(MATRIX2MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(MATRIX2MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(MATRIX3MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(MATRIX3MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(MATRIX4MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(MATRIX4MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(KEYEDARCHMAPID) == *loadedArchiveFromGeneratedFile->GetVariant(KEYEDARCHMAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(INT8MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(INT8MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(UINT8MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(UINT8MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(INT16MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(INT16MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(UINT16MAPID) == *loadedArchiveFromGeneratedFile->GetVariant(UINT16MAPID));
        TEST_VERIFY(*loadedArchive->GetVariant(COLORMAPID) == *loadedArchiveFromGeneratedFile->GetVariant(COLORMAPID));

        testArchive->SetFloat(TESTKEY, 999.0f);
        const VariantType* variantFloatPtr = testArchive->GetVariant(TESTKEY);

        testArchive->SetString(TESTKEY, "test string");
        const VariantType* variantStringPtr = testArchive->GetVariant(TESTKEY);

        VariantType variant;
        variant.SetBool(false);
        testArchive->SetVariant(TESTKEY, std::move(variant));
        const VariantType* variantPtr = testArchive->GetVariant(TESTKEY);

        TEST_VERIFY(variantFloatPtr == variantStringPtr);
        TEST_VERIFY(variantPtr == variantStringPtr);
        TEST_VERIFY(variant.GetType() == VariantType::TYPE_NONE);
    }

    DAVA_TEST (CheckCopy)
    {
        using namespace DAVA;

        KeyedArchive* arc1root = new KeyedArchive();

        const char* keySub = "sub";
        const char* keyUrl = "url";
        const char* keyInt = "i";
        const char* keyFloat = "d";

        const char* valueUrl = "https://any.com";
        const int64 valueInt = 42LL;
        const double valueFloat64 = 100.500;

        {
            KeyedArchive* arc2sub = new KeyedArchive();
            arc2sub->SetString(keyUrl, valueUrl);
            arc2sub->SetInt64(keyInt, valueInt);
            arc2sub->SetFloat64(keyFloat, valueFloat64);

            arc1root->SetArchive(keySub, arc2sub);

            SafeRelease(arc2sub);
        }

        {
            KeyedArchive* arc2sub = arc1root->GetArchive(keySub);
            TEST_VERIFY(arc2sub != nullptr);
            String cs = arc2sub->GetString(keyUrl);
            TEST_VERIFY(cs == valueUrl);
            int64 i = arc2sub->GetInt64(keyInt);
            TEST_VERIFY(i == valueInt);
            float64 f = arc2sub->GetFloat64(keyFloat);
            TEST_VERIFY(f == valueFloat64);

            // try call operator= for save keyedArchive
            arc1root->SetArchive(keySub, arc2sub);
        }
        // then check again all values
        {
            KeyedArchive* arc2sub = arc1root->GetArchive(keySub);
            TEST_VERIFY(arc2sub != nullptr);
            String cs = arc2sub->GetString(keyUrl);
            TEST_VERIFY(cs == valueUrl);
            int64 i = arc2sub->GetInt64(keyInt);
            TEST_VERIFY(i == valueInt);
            float64 f = arc2sub->GetFloat64(keyFloat);
            TEST_VERIFY(f == valueFloat64);
        }

        SafeRelease(arc1root);
    }
};
