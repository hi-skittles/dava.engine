#include "UnitTests/UnitTests.h"
#include "Math/AABBox3.h"
#include "Utils/Random.h"

using namespace DAVA;

DAVA_TESTCLASS (RayMathTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("AABBox3.cpp")
    END_FILES_COVERED_BY_TESTS()

    DAVA_TEST (AABBox3Test)
    {
        AABBox3 emptyBox;
        Vector3 center(0.0f, 0.0f, 0.0f);
        float largeBoxSize = 10.0f;
        float smallBoxSize = 1.0f;
        AABBox3 largeBox(center, largeBoxSize);
        AABBox3 smallBox(center, smallBoxSize);

        TEST_VERIFY(largeBox.IntersectsWithBox(smallBox));
        TEST_VERIFY(smallBox.IntersectsWithBox(smallBox));

        TEST_VERIFY(emptyBox.IsEmpty() == true);
        TEST_VERIFY(smallBox.IsEmpty() == false);

        TEST_VERIFY(largeBox.IsInside(center));
        TEST_VERIFY(smallBox.IsInside(center));

        TEST_VERIFY(largeBox.IsInside(smallBox) == true);
        TEST_VERIFY(smallBox.IsInside(largeBox) == false);

        Vector3 testCenter = largeBox.GetCenter();
        TEST_VERIFY(FLOAT_EQUAL(center.x, testCenter.x) && FLOAT_EQUAL(center.y, testCenter.y) && FLOAT_EQUAL(center.z, testCenter.z));

        Vector3 testSize = largeBox.GetSize();
        TEST_VERIFY(FLOAT_EQUAL(largeBoxSize, testSize.x) && FLOAT_EQUAL(largeBoxSize, testSize.y) && FLOAT_EQUAL(largeBoxSize, testSize.z));

        {
            AABBox3 testBox(center, largeBoxSize);
            TEST_VERIFY(testBox == largeBox);
            TEST_VERIFY(testBox != smallBox);
        }

        AABBox3 transformedBox;
        smallBox.GetTransformedBox(Matrix4::MakeRotation(Vector3::UnitX, -PI / 2.0f), transformedBox);
        TEST_VERIFY(FLOAT_EQUAL(transformedBox.min.x, smallBox.min.x));
        TEST_VERIFY(FLOAT_EQUAL(transformedBox.min.y, smallBox.min.y));
        TEST_VERIFY(FLOAT_EQUAL(transformedBox.min.z, smallBox.min.z));
        TEST_VERIFY(FLOAT_EQUAL(transformedBox.max.x, smallBox.max.x));
        TEST_VERIFY(FLOAT_EQUAL(transformedBox.max.y, smallBox.max.y));
        TEST_VERIFY(FLOAT_EQUAL(transformedBox.max.z, smallBox.max.z));
    }

    DAVA_TEST (RayAABBoxCollisionTest)
    {
        {
            AABBox3 box(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.5f, 0.5f, 0.5f));

            Ray3 bordersTest[] =
            {
              // along all sides tests
              Ray3(Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3(Vector3(0.5f, 0.5f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3(Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3(Vector3(0.5f, 0.5f, 1.0f), Vector3(0.0f, 0.0f, -2.0f)),
              Ray3(Vector3(0.0f, -1.0f, 0.0f), Vector3(0.0f, 2.0f, 0.0f)),
              Ray3(Vector3(0.5f, -1.0f, 0.5f), Vector3(0.0f, 2.0f, 0.0f)),
              Ray3(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, -2.0f, 0.0f)),
              Ray3(Vector3(0.5f, 1.0f, 0.5f), Vector3(0.0f, -2.0f, 0.0f)),
              Ray3(Vector3(-1.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f)),
              Ray3(Vector3(-1.0f, 0.5f, 0.5f), Vector3(2.0f, 0.0f, 0.0f)),
              Ray3(Vector3(1.0f, 0.0f, 0.0f), Vector3(-2.0f, 0.0f, 0.0f)),
              Ray3(Vector3(1.0f, 0.5f, 0.5f), Vector3(-2.0f, 0.0f, 0.0f)),
              Ray3(Vector3(-0.5f, -0.5f, -0.5f), Vector3(2.0f, 2.0f, 2.0f)),
              Ray3(Vector3(1.0f, 1.0f, 1.0f), Vector3(-2.0f, -2.0f, -2.0f)),
            };
            float results[][2]
            {
              { 0.5f, 0.75f },
              { 0.5f, 0.75f },
              { 0.25f, 0.5f },
              { 0.25f, 0.5f },

              { 0.5f, 0.75f },
              { 0.5f, 0.75f },
              { 0.25f, 0.5f },
              { 0.25f, 0.5f },

              { 0.5f, 0.75f },
              { 0.5f, 0.75f },
              { 0.25f, 0.5f },
              { 0.25f, 0.5f },

              { 0.25f, 0.5f },
              { 0.25f, 0.5f },
            };

            // Check borders
            for (uint32 k = 0; k < sizeof(bordersTest) / sizeof(Ray3); ++k)
            {
                float32 tMin, tMax;
                TEST_VERIFY(Intersection::RayBox(bordersTest[k], box) == true);

                TEST_VERIFY(Intersection::RayBox(bordersTest[k], box, tMin) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, results[k][0]));

                TEST_VERIFY(Intersection::RayBox(bordersTest[k], box, tMin, tMax) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, results[k][0]));
                TEST_VERIFY(FLOAT_EQUAL(tMax, results[k][1]));

                Ray3Optimized rayOpt(bordersTest[k].origin, bordersTest[k].direction);

                TEST_VERIFY(Intersection::RayBox(rayOpt, box, tMin, tMax) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, results[k][0]));
                TEST_VERIFY(FLOAT_EQUAL(tMax, results[k][1]));
            }

            const std::pair<Vector3, Vector3> segmentsTest[] =
            {
              { Vector3(-2.0f, 0.0f, 0.0f), Vector3(+2.0f, 0.0f, 0.0f) },
              { Vector3(0.0f, -2.0f, 0.0f), Vector3(0.0f, +2.0f, 0.0f) },
              { Vector3(0.0f, 0.0f, -2.0f), Vector3(0.0f, 0.0f, +2.0f) },

              { Vector3(-2.0f, -2.0f, -2.0f), Vector3(2.0f, 2.0f, 2.0f) },
              { Vector3(2.0f, 2.0f, 2.0f), Vector3(-2.0f, -2.0f, -2.0f) },
            };
            for (uint32 i = 0, e = static_cast<uint32_t>(sizeof(segmentsTest) / sizeof(segmentsTest[0])); i < e; ++i)
            {
                float32 tMin = 0.0f;
                float32 tMax = 0.0f;
                TEST_VERIFY(Intersection::SegmentBox(segmentsTest[i].first, segmentsTest[i].second, box, tMin, tMax) == true);
            }
        }
        {
            AABBox3 box(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));

            Ray3 bordersTest[] =
            {
              // along all sides tests
              Ray3(Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
              Ray3(Vector3(0.5f, 0.5f, -1.0f), Vector3(0.0f, 0.0f, 2.0f)),
            };
            float results[][2]
            {
              { 0.5f, 1.0f },
              { 0.5f, 1.0f },
            };

            // Check borders
            for (uint32 k = 0; k < sizeof(bordersTest) / sizeof(Ray3); ++k)
            {
                float32 tMin, tMax;
                TEST_VERIFY(Intersection::RayBox(bordersTest[k], box, tMin));

                TEST_VERIFY(Intersection::RayBox(bordersTest[k], box, tMin) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, results[k][0]));

                TEST_VERIFY(Intersection::RayBox(bordersTest[k], box, tMin, tMax) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, results[k][0]));
                TEST_VERIFY(FLOAT_EQUAL(tMax, results[k][1]));

                Ray3Optimized rayOpt(bordersTest[k].origin, bordersTest[k].direction);

                TEST_VERIFY(Intersection::RayBox(rayOpt, box, tMin, tMax) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, results[k][0]));
                TEST_VERIFY(FLOAT_EQUAL(tMax, results[k][1]));
            }
        }

        AABBox3 box2(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.3f, 1.2f, 1.5f));

        Ray3 sideRayBase[] =
        {
          Ray3(Vector3(box2.min.x, box2.min.y, box2.min.z - 0.5f), Vector3(0.0f, 0.0f, 1.0f)),
          Ray3(Vector3(box2.min.x, box2.min.y, box2.max.z + 0.5f), Vector3(0.0f, 0.0f, -1.0f)),

          Ray3(Vector3(box2.min.x, box2.min.y - 0.5f, box2.min.z), Vector3(0.0f, 1.0f, 0.0f)),
          Ray3(Vector3(box2.min.x, box2.max.y + 0.5f, box2.min.z), Vector3(0.0f, -1.0f, 0.0f)),

          Ray3(Vector3(box2.min.x - 0.5f, box2.min.y, box2.min.z), Vector3(1.0f, 0.0f, 0.0f)),
          Ray3(Vector3(box2.max.x + 0.5f, box2.min.y, box2.min.z), Vector3(-1.0f, 0.0f, 0.0f)),
        };

        Vector3 randShift[] =
        {
          Vector3(1.0f, 1.0f, 0.0f),
          Vector3(1.0f, 1.0f, 0.0f),
          Vector3(1.0f, 0.0f, 1.0f),
          Vector3(1.0f, 0.0f, 1.0f),
          Vector3(0.0f, 1.0f, 1.0f),
          Vector3(0.0f, 1.0f, 1.0f),
        };
        Vector3 size = box2.GetSize();

        // Random sample each side (inside)
        for (uint32 side = 0; side < 6; ++side)
        {
            for (uint32 t = 0; t < 1000; ++t)
            {
                float32 r = Random::Instance()->RandFloat32InBounds(0.0f, 1.0f);
                Ray3 ray = sideRayBase[side];
                ray.origin += randShift[side] * size * r;
                float32 tMin, tMax;
                TEST_VERIFY(Intersection::RayBox(ray, box2) == true);

                TEST_VERIFY(Intersection::RayBox(ray, box2, tMin) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, 0.5f));

                TEST_VERIFY(Intersection::RayBox(ray, box2, tMin, tMax) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, 0.5f));

                Ray3Optimized rayOpt(ray.origin, ray.direction);
                TEST_VERIFY(Intersection::RayBox(rayOpt, box2, tMin, tMax) == true);
                TEST_VERIFY(FLOAT_EQUAL(tMin, 0.5f));
            }

            for (uint32 t = 0; t < 1000; ++t)
            {
                float32 r = Random::Instance()->RandFloat32InBounds(0.1f, 1.0f);
                Ray3 ray = sideRayBase[side];
                ray.origin += -randShift[side] * size * r;
                float32 tMin, tMax;
                TEST_VERIFY(Intersection::RayBox(ray, box2) == false);

                TEST_VERIFY(Intersection::RayBox(ray, box2, tMin) == false);

                TEST_VERIFY(Intersection::RayBox(ray, box2, tMin, tMax) == false);

                Ray3Optimized rayOpt(ray.origin, ray.direction);
                TEST_VERIFY(Intersection::RayBox(rayOpt, box2, tMin, tMax) == false);
            }
        }

        // Big distance test

        // Test end of ray inside box

        // Test whole ray inside box
        {
            Ray3 ray(Vector3(0.5f, 0.5f, 0.5f), Vector3(0.1f, 0.0f, 0.0f));
            AABBox3 box(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));
            float32 tMin, tMax;
            TEST_VERIFY(Intersection::RayBox(ray, box) == true);

            TEST_VERIFY(Intersection::RayBox(ray, box, tMin) == true);
            TEST_VERIFY(tMin == 5.0f);

            TEST_VERIFY(Intersection::RayBox(ray, box, tMin, tMax) == true);
            TEST_VERIFY(tMin == -5.0f);
            TEST_VERIFY(tMax == 5.0f);
        }

        {
            Ray3 ray(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f));
            AABBox3 box(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));
            float32 tMin, tMax;

            TEST_VERIFY(Intersection::RayBox(ray, box) == true);

            TEST_VERIFY(Intersection::RayBox(ray, box, tMin) == true);
            TEST_VERIFY(tMin == 0.0f);

            TEST_VERIFY(Intersection::RayBox(ray, box, tMin, tMax) == true);
            TEST_VERIFY(tMin == 0.0f);
            TEST_VERIFY(tMax == 0.5f);
        }
    };

    DAVA_TEST (TriangleAABBoxCollisionTest)
    {
        AABBox3 box(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));

        Vector3 p0, p1, p2;
        p0 = Vector3(1.0f, 3.0f, 1.0f);
        p1 = Vector3(3.0f, 1.0f, 1.0f);
        p2 = Vector3(1.0f, 1.0f, 3.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true);

        p0 = Vector3(-1.0f, -3.0f, -1.0f);
        p1 = Vector3(-3.0f, -1.0f, -1.0f);
        p2 = Vector3(-1.0f, -1.0f, -3.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == false);

        p0 = Vector3(0.0f, 0.0f, 0.0f);
        p1 = Vector3(-3.0f, -1.0f, -1.0f);
        p2 = Vector3(-1.0f, -1.0f, -3.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true); // Edge should be count as intersection

        p0 = Vector3(-1000.0f, -1000.0f, -1000.0f);
        p1 = Vector3(1000.0f, 1000.0f, 1000.0f);
        p2 = Vector3(1000.0f, 1000.0f, 1000.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true);

        p0 = Vector3(5.0f, 1000.0f, 1000.0f);
        p1 = Vector3(5.0f, -1000.0f, 1000.0f);
        p2 = Vector3(5.0f, 1000.0f, -1000.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true);

        p0 = Vector3(-50.0f, 5.0f, 5.0f);
        p1 = Vector3(100.0f, 5.0f, 5.0f);
        p2 = Vector3(101.0f, 5.0f, 5.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true);

        p0 = Vector3(150.0f, 5.0f, 5.0f);
        p1 = Vector3(-50.0f, 5.0f, 5.0f);
        p2 = Vector3(-51.0f, 5.0f, 5.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true);

        // The difference in results is just the math of the intersection test.
        p0 = Vector3(1500.0f, 5.0f, 5.0f);
        p1 = Vector3(10.0003f, 5.0f, 5.0f);
        p2 = Vector3(10.0003f, 5.0f, 5.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == false); // result is false because it's outside the box

        p0 = Vector3(1500.0f, 5.0f, 5.0f);
        p1 = Vector3(10.0001f, 5.0f, 5.0f);
        p2 = Vector3(10.0001f, 5.0f, 5.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == false);

        p0 = Vector3(20.0f, 5.0f, 5.0f);
        p1 = Vector3(10.0001f, 5.0f, 5.0f);
        p2 = Vector3(10.0001f, 5.0f, 5.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == false); // here origin is closer and same outside result tend to be inside.

        p0 = Vector3(-5000.0, -5000.0, -5000.0);
        p1 = Vector3(5000.0f, 0.0f, 5000.0f);
        p1 = Vector3(0.0f, 5000.0f, 5000.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true);
    }

    DAVA_TEST (TriangleAABBoxFloatPrecisionProblemTest)
    {
        AABBox3 box(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 10.0f, 10.0f));

        Vector3 p0, p1, p2;
        p0 = Vector3(1.0f, 3.0f, 1.0f);
        p1 = Vector3(3.0f, 1.0f, 1.0f);
        p2 = Vector3(1.0f, 1.0f, 3.0f);
        TEST_VERIFY(Intersection::BoxTriangle(box, p0, p1, p2) == true);
    }
};
