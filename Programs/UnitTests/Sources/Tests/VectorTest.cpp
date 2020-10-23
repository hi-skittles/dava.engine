#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"
#include "Math/Vector.h"

using namespace DAVA;

DAVA_TESTCLASS (VectorTest)
{
    DAVA_TEST (Vector4ToVector3Test)
    {
        Vector4 mutableSource(0.1f, 0.2f, 0.3f, 0.4f);
        Vector3& mutableVector3 = mutableSource.GetVector3();
        TEST_VERIFY((mutableVector3.x == mutableSource.x) && (mutableVector3.y == mutableSource.y) && (mutableVector3.z == mutableSource.z))

        const Vector4 immutableSource(0.3f, 0.2f, 0.1f, 0.0f);
        const Vector3& immutableVector3 = immutableSource.GetVector3();
        TEST_VERIFY((immutableVector3.x == immutableSource.x) && (immutableVector3.y == immutableSource.y) && (immutableVector3.z == immutableSource.z))
    }
}
;
