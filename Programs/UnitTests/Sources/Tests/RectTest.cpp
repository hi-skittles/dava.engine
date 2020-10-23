#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Math/Rect.h"

using namespace DAVA;

DAVA_TESTCLASS (RectTest)
{
    //this test from Qt: void tst_QRect::containsRectF_data()
    DAVA_TEST (RectContainsTest)
    {
        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(2.0f, 2.0f, 6.0f, 6.0f)));
        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(0.0f, 0.0f, 10.0f, 10.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(2.0f, 2.0f, 10.0f, 10.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(20.0f, 20.0f, 10.0f, 10.0f)));

        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectContains(Rect(2.0f, 2.0f, 6.0f, 6.0f)));
        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectContains(Rect(0.0f, 0.0f, 10.0f, 10.0f)));
        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectContains(Rect(2.0f, 2.0f, 10.0f, 10.0f)));
        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectContains(Rect(20.0f, 20.0f, 10.0f, 10.0f)));

        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(8.0f, 8.0f, -6.0f, -6.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(10.0f, 10.0f, -10.0f, -10.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(12.0f, 12.0f, -10.0f, -10.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectContains(Rect(30.0f, 30.0f, -10.0f, -10.0f)));

        TEST_VERIFY(Rect(-1.0f, -1.0f, 10.0f, 10.0f).RectContains(Rect()));
        TEST_VERIFY(!Rect().RectContains(Rect(0.0f, 0.0f, 10.0f, 10.0f)));
        TEST_VERIFY(Rect().RectContains(Rect()));
    }

    //this test from Qt: void tst_QRect::intersectsRectF_data()
    DAVA_TEST (RectIntersectsTest)
    {
        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(2.0f, 2.0f, 6.0f, 6.0f)));
        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(0.0f, 0.0f, 10.0f, 10.0f)));
        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(2.0f, 2.0f, 10.0f, 10.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(20.0f, 20.0f, 10.0f, 10.0f)));

        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectIntersects(Rect(2.0f, 2.0f, 6.0f, 6.0f)));
        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectIntersects(Rect(0.0f, 0.0f, 10.0f, 10.0f)));
        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectIntersects(Rect(2.0f, 2.0f, 10.0f, 10.0f)));
        TEST_VERIFY(!Rect(10.0f, 10.0f, -10.0f, -10.0f).RectIntersects(Rect(20.0f, 20.0f, 10.0f, 10.0f)));

        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(8.0f, 8.0f, -6.0f, -6.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(10.0f, 10.0f, -10.0f, -10.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(12.0f, 12.0f, -10.0f, -10.0f)));
        TEST_VERIFY(!Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(30.0f, 30.0f, -10.0f, -10.0f)));

        TEST_VERIFY(Rect(15.0f, 15.0f, 0.0f, 0.0f).RectIntersects(Rect(10.0f, 10.0f, 10.0f, 10.0f)));
        TEST_VERIFY(Rect(10.0f, 10.0f, 10.0f, 10.0f).RectIntersects(Rect(15.0f, 15.0f, 0.0f, 0.0f)));
        TEST_VERIFY(Rect().RectIntersects(Rect()));

        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(10.0f, 10.0f, 10.0f, 10.0f)));
        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(0.0f, 10.0f, 10.0f, 10.0f)));
        TEST_VERIFY(Rect(0.0f, 0.0f, 10.0f, 10.0f).RectIntersects(Rect(10.0f, 0.0f, 10.0f, 10.0f)));
    }
}
;
