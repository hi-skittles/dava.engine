#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

namespace
{
float32 SquareDist(const Matrix4& m1, const Matrix4& m2)
{
    float32 result = 0.0f;
    for (uint32 i = 0; i < 4; ++i)
        for (uint32 j = 0; j < 4; ++j)
            result += Abs(m1(i, j) - m2(i, j)) * Abs(m1(i, j) - m2(i, j));

    return result;
}
}

DAVA_TESTCLASS (MathTest)
{
    DAVA_TEST (MatrixTestFunction)
    {
        TEST_VERIFY(TestMatrixDecomposition(Matrix4::MakeTranslation(Vector3(10.0f, 0.0f, 0.0f))) < 0.0001f);
        TEST_VERIFY(TestMatrixDecomposition(Matrix4::MakeRotation(Vector3(1.0f, 0.0f, 0.0f), -PI_05)) < 0.0001f);
        TEST_VERIFY(TestMatrixDecomposition(Matrix4::MakeScale(Vector3(3.0f, 3.0f, 3.0f))) < 0.0001f);

        Vector3 axis(0.0f, 1.0f, 1.0f);
        axis.Normalize();
        TEST_VERIFY(
        TestMatrixDecomposition(
        Matrix4::MakeTranslation(Vector3(10.0f, 0.0f, 0.0f)) *
        Matrix4::MakeRotation(axis, -PI_05 * 0.25f) *
        Matrix4::MakeScale(Vector3(3.0f, 3.0f, 3.0f))) < 0.0001f);
    }

    float32 TestMatrixDecomposition(const Matrix4& mat)
    {
        Vector3 position, scale;
        Quaternion rotation;
        mat.Decomposition(position, scale, rotation);

        Matrix4 reconstructedMatrix = rotation.GetMatrix() * Matrix4::MakeScale(scale);
        reconstructedMatrix.SetTranslationVector(position);
        return SquareDist(reconstructedMatrix, mat);
    }
}
;
