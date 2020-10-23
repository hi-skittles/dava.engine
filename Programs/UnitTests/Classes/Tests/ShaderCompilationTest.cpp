#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (ShaderCompilationTest)
{
    DAVA_TEST (CompilationTest)
    {
        TEST_VERIFY(TestMaterial(NMaterialName::TEXTURED_OPAQUE));
        TEST_VERIFY(TestMaterial(NMaterialName::TEXTURED_OPAQUE));
        TEST_VERIFY(TestMaterial(NMaterialName::DETAIL_OPAQUE));
        TEST_VERIFY(TestMaterial(NMaterialName::TEXTURE_LIGHTMAP_OPAQUE));

        TEST_VERIFY(TestMaterial(NMaterialName::TILE_MASK));
        if (rhi::DeviceCaps().isInstancingSupported && rhi::DeviceCaps().isVertexTextureUnitsSupported)
        {
            TEST_VERIFY(TestMaterial(NMaterialName::TILE_MASK, { NMaterialFlagName::FLAG_LANDSCAPE_USE_INSTANCING }));
            TEST_VERIFY(TestMaterial(NMaterialName::TILE_MASK, { NMaterialFlagName::FLAG_LANDSCAPE_USE_INSTANCING, NMaterialFlagName::FLAG_LANDSCAPE_LOD_MORPHING }));
        }

        TEST_VERIFY(TestMaterial(NMaterialName::WATER_PER_PIXEL_REAL_REFLECTIONS));
        TEST_VERIFY(TestMaterial(NMaterialName::WATER_PER_PIXEL_CUBEMAP_ALPHABLEND));
        TEST_VERIFY(TestMaterial(NMaterialName::WATER_PER_VERTEX_CUBEMAP_DECAL));

        TEST_VERIFY(TestMaterial(NMaterialName::SPEEDTREE_ALPHABLEND_ALPHATEST));
        TEST_VERIFY(TestMaterial(NMaterialName::SPHERICLIT_TEXTURED_OPAQUE));
        TEST_VERIFY(TestMaterial(NMaterialName::SPHERICLIT_SPEEDTREE_ALPHABLEND_ALPHATEST));

        TEST_VERIFY(TestMaterial(NMaterialName::SHADOW_VOLUME));
        TEST_VERIFY(TestMaterial(NMaterialName::SHADOW_VOLUME, { NMaterialFlagName::FLAG_HARD_SKINNING }));

        TEST_VERIFY(TestMaterial(NMaterialName::SILHOUETTE));
        TEST_VERIFY(TestMaterial(NMaterialName::SILHOUETTE, { NMaterialFlagName::FLAG_HARD_SKINNING }));

        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_PIXEL_OPAQUE));
        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_PIXEL_OPAQUE, { NMaterialFlagName::FLAG_HARD_SKINNING }));
        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_PIXEL_OPAQUE, { NMaterialFlagName::FLAG_SOFT_SKINNING }));

        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_PIXEL_FAST_OPAQUE));
        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_PIXEL_FAST_OPAQUE, { NMaterialFlagName::FLAG_HARD_SKINNING }));
        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_PIXEL_FAST_OPAQUE, { NMaterialFlagName::FLAG_SOFT_SKINNING }));

        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_VERTEX_OPAQUE));
        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_VERTEX_OPAQUE, { NMaterialFlagName::FLAG_HARD_SKINNING }));
        TEST_VERIFY(TestMaterial(NMaterialName::NORMALIZED_BLINN_PHONG_PER_VERTEX_OPAQUE, { NMaterialFlagName::FLAG_SOFT_SKINNING }));
    }

    bool TestMaterial(const FastName& fxName, const Vector<FastName>& flags = Vector<FastName>(), const FastName& pass = PASS_FORWARD)
    {
        ScopedPtr<NMaterial> material(new NMaterial());
        material->SetFXName(fxName);

        for (const FastName& flag : flags)
            material->AddFlag(flag, 1);

        return material->PreBuildMaterial(PASS_FORWARD);
    }
};
