#include "UnitTests/UnitTests.h"
#include "Render/Highlevel/GeometryOctTree.h"
#include "Render/Highlevel/GeometryGenerator.h"
#include <random>

#include <stdlib.h>
#include <math.h>

using namespace DAVA;

DAVA_TESTCLASS (GeometryOctTreeTest)
{
#pragma warning(disable : 4723)

    DAVA_TEST (BasicOctTreeRayTest)
    {
        for (uint32 t = 0; t < 5; ++t)
        {
            const uint32 segments[5] = { 1, 2, 3, 30, 50 }; // Check 100, 100 is failing

            Map<FastName, float32> options = {
                { FastName("segments.x"), static_cast<float32>(segments[t]) },
                { FastName("segments.y"), static_cast<float32>(segments[t]) },
                { FastName("segments.z"), static_cast<float32>(segments[t]) }
            };

            PolygonGroup* geometry = GeometryGenerator::GenerateBox(AABBox3(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f)), options);
            geometry->GenerateGeometryOctTree();
            GeometryOctTree* geoOctTree = geometry->GetGeometryOctTree();

            Ray3Optimized rays[] =
            {
              Ray3Optimized(Vector3(0.49999f, 0.49999f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3Optimized(Vector3(0.5f, 0.5f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3Optimized(Vector3(0.50001f, 0.50001f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3Optimized(Vector3(0.49999f, 0.50001f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3Optimized(Vector3(0.50001f, 0.49999f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3Optimized(Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3Optimized(Vector3(1.0f, 1.0f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),

              Ray3Optimized(Vector3(0.49999f, 0.49999f, 2.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3Optimized(Vector3(0.5f, 0.5f, 2.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3Optimized(Vector3(0.50001f, 0.50001f, 2.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3Optimized(Vector3(0.49999f, 0.50001f, 2.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3Optimized(Vector3(0.50001f, 0.49999f, 2.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3Optimized(Vector3(0.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3Optimized(Vector3(1.0f, 1.0f, 2.0f), Vector3(0.0f, 0.0f, -2.0f)),
            };

            Vector<Ray3Optimized> finalRays;
            for (auto ray : rays)
            {
                finalRays.push_back(ray);
                finalRays.push_back(Ray3Optimized(ray.origin.xzy(), ray.direction.xzy()));
                finalRays.push_back(Ray3Optimized(ray.origin.zxy(), ray.direction.zxy()));
            }

            for (auto ray : finalRays)
            {
                uint32 triIndex;
                float32 resultT;
                TEST_VERIFY(geoOctTree->IntersectionWithRay(ray, resultT, triIndex) == true);
                TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

                TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray, resultT, triIndex) == true);
                TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));
            }

            uint32 triIndex;
            float32 resultT;

            Ray3Optimized rayf(Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(rayf, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(rayf, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            Ray3Optimized ray(Vector3(0.49999f, 0.49999f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(ray, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            Ray3Optimized ray2(Vector3(0.5f, 0.5f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(ray2, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray2, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            Ray3Optimized ray3(Vector3(0.51f, 0.51f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(ray3, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray3, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            Ray3Optimized ray4(Vector3(0.49f, 0.51f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(ray4, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray4, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            Ray3Optimized ray5(Vector3(0.51f, 0.49f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(ray5, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray5, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            Ray3Optimized ray6(Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(ray6, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray6, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            Ray3Optimized ray7(Vector3(1.0f, 1.0f, -1.0f), Vector3(0.0f, 0.0f, 2.0f));
            TEST_VERIFY(geoOctTree->IntersectionWithRay(ray7, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            TEST_VERIFY(geoOctTree->IntersectionWithRay2(ray7, resultT, triIndex) == true);
            TEST_VERIFY(FLOAT_EQUAL(resultT, 0.5f));

            SafeRelease(geometry);
        }
    }
};
