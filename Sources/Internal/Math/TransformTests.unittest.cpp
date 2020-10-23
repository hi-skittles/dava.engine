#include "Math/Transform.h"
#include "Math/TransformUtils.h"

#include "Logger/Logger.h"

#include <UnitTests/UnitTests.h>

using namespace DAVA;

const float32 TEST_EPSILON = 1e-6f;

namespace TransformTestDetails
{
template <class Type>
bool AreEqual(const Type& t1, const Type& t2, size_t count, float32 epsilon)
{
    Logger::Info("[%s] >>>>> %d", __FUNCTION__, static_cast<int>(count));

    for (size_t i = 0; i < count; ++i)
    {
        Logger::Info("[%d]  %0.6f  <>  %0.6f", i, t1.data[i], t2.data[i]);

        if (fabs(t1.data[i] - t2.data[i]) > epsilon)
        {
            Logger::Info("[%s] <<<<< false", __FUNCTION__);
            return false;
        }
    }
    Logger::Info("[%s] <<<<< true", __FUNCTION__);
    return true;
}

bool AreEqual(const Transform& t1, const Transform& t2, float32 epsilon)
{
    bool retTranslation = AreEqual(t1.GetTranslation(), t2.GetTranslation(), 3, epsilon);
    bool retScale = AreEqual(t1.GetScale(), t2.GetScale(), 3, epsilon);
    bool retRotation = AreEqual(t1.GetRotation(), t2.GetRotation(), 4, epsilon);

    return retTranslation && retScale && retRotation;
}
}

DAVA_TESTCLASS (TransformTests)
{
    DAVA_TEST (MatrixRotationTest)
    {
        auto testRotation = [](const Vector3& rotationVector)
        {
            Matrix4 matX = Matrix4::MakeRotation(Vector3::UnitX, rotationVector.x);
            Matrix4 matY = Matrix4::MakeRotation(Vector3::UnitY, rotationVector.y);
            Matrix4 matZ = Matrix4::MakeRotation(Vector3::UnitZ, rotationVector.z);
            Matrix4 matrix = matX * matY * matZ;

            Vector3 tp, ts, newRotation;
            matrix.Decomposition(tp, ts, newRotation);
            TEST_VERIFY(TransformTestDetails::AreEqual(rotationVector, newRotation, 3, 0.00001f) == true);
        };

        testRotation(Vector3(0.4f, 0.0f, 0.0f));
        testRotation(Vector3(0.0f, 0.3f, 0.0f));
        testRotation(Vector3(0.0f, 0.0f, 0.2f));

        testRotation(Vector3(0.4f, 0.3f, 0.f));
        testRotation(Vector3(0.4f, 0.0f, 0.2f));
        testRotation(Vector3(0.0f, 0.3f, 0.2f));

        testRotation(Vector3(0.4f, 0.3f, 0.2f));
    }

    DAVA_TEST (QuaternionTest)
    {
        // Create from euler
        Vector3 euler(0.3f, 0.2f, 0.1f);
        Matrix4 matX = Matrix4::MakeRotation(Vector3::UnitX, euler.x);
        Matrix4 matY = Matrix4::MakeRotation(Vector3::UnitY, euler.y);
        Matrix4 matZ = Matrix4::MakeRotation(Vector3::UnitZ, euler.z);
        Matrix4 matrix = matX * matY * matZ;

        { //check with matrix
            Vector3 tp, ts, decomposedEuler, decomposedEulerX, decomposedEulerY, decomposedEulerZ;
            matX.Decomposition(tp, ts, decomposedEulerX);
            matY.Decomposition(tp, ts, decomposedEulerY);
            matZ.Decomposition(tp, ts, decomposedEulerZ);
            matrix.Decomposition(tp, ts, decomposedEuler);

            TEST_VERIFY(TransformTestDetails::AreEqual(decomposedEulerX, Vector3(0.3f, 0.0f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(decomposedEulerY, Vector3(0.0f, 0.2f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(decomposedEulerZ, Vector3(0.0f, 0.0f, 0.1f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(decomposedEuler, euler, 3, 0.00001f) == true);
        }

        { // from euler
            Quaternion rotationX = Quaternion::MakeRotation(Vector3(0.3f, 0.0f, 0.0f));
            Quaternion rotationY = Quaternion::MakeRotation(Vector3(0.0f, 0.2f, 0.0f));
            Quaternion rotationZ = Quaternion::MakeRotation(Vector3(0.0f, 0.0f, 0.1f));

            Vector3 newEulerX = rotationX.GetEuler();
            Matrix4 newMatrixX = rotationX.GetMatrix();

            Vector3 newEulerY = rotationY.GetEuler();
            Matrix4 newMatrixY = rotationY.GetMatrix();

            Vector3 newEulerZ = rotationZ.GetEuler();
            Matrix4 newMatrixZ = rotationZ.GetMatrix();

            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerX, Vector3(0.3f, 0.0f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerY, Vector3(0.0f, 0.2f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerZ, Vector3(0.0f, 0.0f, 0.1f), 3, 0.00001f) == true);

            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixX, matX, 16, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixY, matY, 16, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixZ, matZ, 16, 0.00001f) == true);

            Quaternion rotation = Quaternion::MakeRotation(euler);
            Vector3 newEuler = rotation.GetEuler();
            Matrix4 newMatrix = rotation.GetMatrix();
            TEST_VERIFY(TransformTestDetails::AreEqual(newEuler, euler, 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrix, matrix, 16, 0.00001f) == true);
        }

        { // from matrix
            Quaternion rotationX = Quaternion::MakeRotation(matX);
            Quaternion rotationY = Quaternion::MakeRotation(matY);
            Quaternion rotationZ = Quaternion::MakeRotation(matZ);
            Quaternion rotation = Quaternion::MakeRotation(matrix);

            Vector3 newEulerX = rotationX.GetEuler();
            Matrix4 newMatrixX = rotationX.GetMatrix();

            Vector3 newEulerY = rotationY.GetEuler();
            Matrix4 newMatrixY = rotationY.GetMatrix();

            Vector3 newEulerZ = rotationZ.GetEuler();
            Matrix4 newMatrixZ = rotationZ.GetMatrix();

            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerX, Vector3(0.3f, 0.0f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerY, Vector3(0.0f, 0.2f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerZ, Vector3(0.0f, 0.0f, 0.1f), 3, 0.00001f) == true);

            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixX, matX, 16, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixY, matY, 16, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixZ, matZ, 16, 0.00001f) == true);

            Vector3 newEuler = rotation.GetEuler();
            Matrix4 newMatrix = rotation.GetMatrix();
            TEST_VERIFY(TransformTestDetails::AreEqual(newEuler, euler, 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrix, matrix, 16, 0.00001f) == true);
        }

        { //from rotations
            Quaternion rotationX = Quaternion::MakeRotation(Vector3::UnitX, euler.x);
            Quaternion rotationY = Quaternion::MakeRotation(Vector3::UnitY, euler.y);
            Quaternion rotationZ = Quaternion::MakeRotation(Vector3::UnitZ, euler.z);
            Quaternion rotation = rotationZ * rotationY * rotationX;

            Vector3 newEulerX = rotationX.GetEuler();
            Matrix4 newMatrixX = rotationX.GetMatrix();

            Vector3 newEulerY = rotationY.GetEuler();
            Matrix4 newMatrixY = rotationY.GetMatrix();

            Vector3 newEulerZ = rotationZ.GetEuler();
            Matrix4 newMatrixZ = rotationZ.GetMatrix();

            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerX, Vector3(0.3f, 0.0f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerY, Vector3(0.0f, 0.2f, 0.0f), 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newEulerZ, Vector3(0.0f, 0.0f, 0.1f), 3, 0.00001f) == true);

            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixX, matX, 16, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixY, matY, 16, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrixZ, matZ, 16, 0.00001f) == true);

            Vector3 newEuler = rotation.GetEuler();
            Matrix4 newMatrix = rotation.GetMatrix();
            TEST_VERIFY(TransformTestDetails::AreEqual(newEuler, euler, 3, 0.00001f) == true);
            TEST_VERIFY(TransformTestDetails::AreEqual(newMatrix, matrix, 16, 0.00001f) == true);
        }
    }

    DAVA_TEST (TransformDefaultsTest)
    {
        using namespace TransformTestDetails;

        Transform transform;
        Matrix4 emptyMatrix;

        Vector3 p, s, o;
        emptyMatrix.Decomposition(p, s, o);

        TEST_VERIFY(AreEqual(transform.GetScale(), s, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(emptyMatrix.GetScaleVector(), s, 3, 0.00001f) == true);

        TEST_VERIFY(AreEqual(transform.GetTranslation(), p, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(emptyMatrix.GetTranslationVector(), p, 3, 0.00001f) == true);

        TEST_VERIFY(AreEqual(transform.GetRotation().GetEuler(), o, 3, 0.00001f) == true);
    }

    DAVA_TEST (SimpleTransformTest)
    {
        using namespace TransformTestDetails;

        Transform transform;

        Vector3 scale(2.15f, 2.15f, 2.15f);
        Vector3 translation(4.5f, 2.1f, 5.5f);
        Vector3 euler(0.3f, 0.2f, 0.1f);

        transform.SetScale(scale);
        transform.SetTranslation(translation);
        transform.SetRotation(Quaternion::MakeRotation(euler));
        Matrix4 transformMatrix = TransformUtils::ToMatrix(transform);

        Vector3 tp, ts, to;
        transformMatrix.Decomposition(tp, ts, to);

        Vector3 tp1, ts1;
        Quaternion rot;
        transformMatrix.Decomposition(tp1, ts1, rot);

        TEST_VERIFY(AreEqual(transform.GetScale(), scale, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(transformMatrix.GetScaleVector(), scale, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(ts, scale, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(ts1, scale, 3, 0.00001f) == true);

        TEST_VERIFY(AreEqual(transform.GetTranslation(), translation, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(transformMatrix.GetTranslationVector(), translation, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(tp, translation, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(tp1, translation, 3, 0.00001f) == true);

        TEST_VERIFY(AreEqual(transform.GetRotation(), rot, 4, 0.00001f) == true);
        TEST_VERIFY(AreEqual(transform.GetRotation().GetEuler(), euler, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(rot.GetEuler(), euler, 3, 0.00001f) == true);
        TEST_VERIFY(AreEqual(to, euler, 3, 0.00001f) == true);
    }

    DAVA_TEST (MultiTranslationTest)
    {
        Matrix4 mComposed = Matrix4::MakeTranslation(Vector3(3.f, 4.f, 0.f))
        * Matrix4::MakeTranslation(Vector3(0.f, 0.f, 1.f))
        * Matrix4::MakeTranslation(Vector3(0.5f, 0.5f, 0.5f));

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeTranslation(Vector3(3.f, 4.f, 0.f))
        * TransformUtils::MakeTranslation(Vector3(0.f, 0.f, 1.f))
        * TransformUtils::MakeTranslation(Vector3(0.5f, 0.5f, 0.5f));

        Transform fromMatrix = Transform(mComposed);

        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetTranslation(), tp, 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (MultiScaleTest)
    {
        Matrix4 mComposed = Matrix4::MakeScale(Vector3(3.f, 3.f, 3.f))
        * Matrix4::MakeScale(Vector3(1.1f, 1.1f, 1.1f))
        * Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeScale(Vector3(3.f, 3.f, 3.f))
        * TransformUtils::MakeScale(Vector3(1.1f, 1.1f, 1.1f))
        * TransformUtils::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        Transform fromMatrix = Transform(mComposed);

        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetScale(), ts, 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (MultiRotationTestSimpleX)
    {
        Matrix4 mComposed = Matrix4::MakeRotation(Vector3::UnitX, 0.35f)
        * Matrix4::MakeRotation(Vector3::UnitX, 0.7f)
        * Matrix4::MakeRotation(Vector3::UnitX, 0.222f);

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeRotation(Vector3::UnitX, 0.35f)
        * TransformUtils::MakeRotation(Vector3::UnitX, 0.7f)
        * TransformUtils::MakeRotation(Vector3::UnitX, 0.222f);

        Transform fromMatrix = Transform(mComposed);

        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), fromMatrix.GetRotation().GetEuler(), 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), to, 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (MultiRotationTestSimpleXYZ)
    {
        Matrix4 mComposed = Matrix4::MakeRotation(Vector3::UnitX, 0.1f)
        * Matrix4::MakeRotation(Vector3::UnitY, 0.4f)
        * Matrix4::MakeRotation(Vector3::UnitZ, 0.5f);

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeRotation(Vector3::UnitX, 0.1f)
        * TransformUtils::MakeRotation(Vector3::UnitY, 0.4f)
        * TransformUtils::MakeRotation(Vector3::UnitZ, 0.5f);

        Transform fromMatrix = Transform(mComposed);

        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), fromMatrix.GetRotation().GetEuler(), 3, 0.0002f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), to, 3, 0.0001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (MultiRotationTest)
    {
        Matrix4 mComposed = Matrix4::MakeRotation(Normalize(Vector3(3.f, 4.f, 0.f)), 0.35f)
        * Matrix4::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.7f)
        * Matrix4::MakeRotation(Normalize(Vector3(0.5f, 0.5f, 0.5f)), 0.222f);

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeRotation(Normalize(Vector3(3.f, 4.f, 0.f)), 0.35f)
        * TransformUtils::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.7f)
        * TransformUtils::MakeRotation(Normalize(Vector3(0.5f, 0.5f, 0.5f)), 0.222f);

        Transform fromMatrix = Transform(mComposed);

        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), fromMatrix.GetRotation().GetEuler(), 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), to, 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (MultiRotationTestExt)
    {
        Matrix4 mComposed = Matrix4::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.57f)
        * Matrix4::MakeTranslation(Vector3(0.f, 100.f, 0.f))
        * Matrix4::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), -0.57f);

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.57f)
        * TransformUtils::MakeTranslation(Vector3(0.f, 100.f, 0.f))
        * TransformUtils::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), -0.57f);

        Vector3 testV(1.f, 1.f, 1.f);
        Vector3 testM = testV * mComposed;
        Vector3 testT = TransformUtils::TransformVector(tComposed, testV);

        Transform fromMatrix = Transform(mComposed);

        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetTranslation(), tp, 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), fromMatrix.GetRotation().GetEuler(), 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed.GetRotation().GetEuler(), to, 3, 0.00001f) == true);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (TransformOrderSameAsMatrixTest)
    {
        Matrix4 mComposed = Matrix4::MakeTranslation(Vector3(3.f, 4.f, 0.f))
        * Matrix4::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.92729343f)
        * Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeTranslation(Vector3(3.f, 4.f, 0.f))
        * TransformUtils::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.92729343f)
        * TransformUtils::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        Transform fromMatrix = Transform(mComposed);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (TransformOrderSameAsMatrixTestExt)
    {
        Matrix4 mComposed = Matrix4::MakeTranslation(Vector3(3.f, 4.f, 0.f))
        * Matrix4::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.92729343f)
        * Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f))
        * Matrix4::MakeTranslation(Vector3(2.f, 15.f, 0.4f))
        * Matrix4::MakeRotation(Normalize(Vector3(1.f, 0.f, 1.f)), 0.3567f)
        * Matrix4::MakeScale(Vector3(0.1f, 0.1f, 0.1f));

        Vector3 tp, ts, to;
        mComposed.Decomposition(tp, ts, to);

        Transform tComposed = TransformUtils::MakeTranslation(Vector3(3.f, 4.f, 0.f))
        * TransformUtils::MakeRotation(Normalize(Vector3(0.f, 0.f, 1.f)), 0.92729343f)
        * TransformUtils::MakeScale(Vector3(0.5f, 0.5f, 0.5f))
        * TransformUtils::MakeTranslation(Vector3(2.f, 15.f, 0.4f))
        * TransformUtils::MakeRotation(Normalize(Vector3(1.f, 0.f, 1.f)), 0.3567f)
        * TransformUtils::MakeScale(Vector3(0.1f, 0.1f, 0.1f));

        Transform fromMatrix = Transform(mComposed);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (TransformOrderSameAsMatrixTest2)
    {
        Vector3 axis = Vector3(1.f, 1.f, 1.f);
        axis.Normalize();
        Matrix4 mComposed = Matrix4::MakeTranslation(Vector3(3.f, 4.f, 5.f))
        * Matrix4::MakeRotation(axis, DAVA::PI_05)
        * Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        Transform tComposed = TransformUtils::MakeTranslation(Vector3(3.f, 4.f, 5.f))
        * TransformUtils::MakeRotation(axis, DAVA::PI_05)
        * TransformUtils::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        Transform fromMatrix(mComposed);
        TEST_VERIFY(TransformTestDetails::AreEqual(tComposed, fromMatrix, 0.00001f));
    }

    DAVA_TEST (TransformMultiplyInversedEqualsIdentityTest)
    {
        Transform someTransform(Vector3(3.f, 4.f, 5.f), Vector3(1.f, 1.f, 1.f),
                                Quaternion::MakeRotation(Normalize(Vector3(1.f, 2.f, 3.f)), DAVA::PI_025));

        Transform transformedAndInversed = Transform() * someTransform * TransformUtils::Inverse(someTransform);

        TEST_VERIFY(TRANSFORM_EQUAL_EPS(transformedAndInversed, Transform(), TEST_EPSILON));
    }

    DAVA_TEST (TransformVectorCorrectlyTransformedTest)
    {
        Vector3 toTransform(1.f, 1.f, 0.f);

        Transform transformation(Vector3(2.f, 0.f, 0.f), Vector3(1.f, 1.f, 1.f),
                                 Quaternion::MakeRotation(Vector3::UnitZ, DAVA::PI_025));

        Vector3 resultVector = TransformUtils::TransformVector(transformation, toTransform);

        TEST_VERIFY(VECTOR_EQUAL_EPS(resultVector, Vector3(2.f, DAVA::SquareRootFloat(2.f), 0.f), TEST_EPSILON));
    }
};
