#include <gtest/gtest.h>

#include <csics/linalg/Matrix.hpp>

TEST(CSICSLinAlgTests, MatrixBasicTest) {
    using namespace csics::linalg;

    Matrix<double, 2, 2> m1{1.0, 2.0, 3.0, 4.0};
    Matrix<double, 2, 2> m2{5.0, 6.0, 7.0, 8.0};

    ASSERT_NE(m1, m2);
    static_assert(
        MatrixCompatible<Matrix<double, 2, 2>&, Matrix<double, 2, 2>&>);
    auto m3 = m1 + m2;
    Matrix<double, 2, 2> expected{{6.0, 8.0}, {10.0, 12.0}};
    ASSERT_EQ(m3, expected);
    auto m4 = m2 - m1;
    Matrix<double, 2, 2> expected_sub{{4.0, 4.0}, {4.0, 4.0}};
    ASSERT_EQ(m4, expected_sub);

    auto m5 = m1 * 2.0;
    Matrix<double, 2, 2> expected_mul{{2.0, 4.0}, {6.0, 8.0}};
    ASSERT_EQ(m5, expected_mul);
    auto m6 = 2.0 * m1;
    ASSERT_EQ(m6, expected_mul);
    auto m7 = m1 / 2.0;
    Matrix<double, 2, 2> expected_div{{0.5, 1.0}, {1.5, 2.0}};
    ASSERT_EQ(m7, expected_div);
};
