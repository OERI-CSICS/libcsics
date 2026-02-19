#include <gtest/gtest.h>
#include <csics/linalg/Vec.hpp>


TEST(CSICSLinAlgTests, Vec2BasicTest) {
    using namespace csics::linalg;

    Vec2<double> v1{1.0, 2.0};
    Vec2<double> v2{3.0, 4.0};

    ASSERT_NE(v1, v2);
    auto v3 = v1 + v2;
    Vec2<double> expected{4.0, 6.0};
    ASSERT_EQ(v3, expected); 
    auto v4 = v2 - v1;
    Vec2<double> expected_sub{2.0, 2.0};
    ASSERT_EQ(v4, expected_sub);

    auto v5 = v1 * 2.0;
    Vec2<double> expected_mul{2.0, 4.0};
    ASSERT_EQ(v5, expected_mul);
    auto v6 = 2.0 * v1;
    ASSERT_EQ(v6, expected_mul);
    auto v7 = v1 / 2.0;
    Vec2<double> expected_div{0.5, 1.0};
    ASSERT_EQ(v7, expected_div);

};

// repeat for Vec3
TEST(CSICSLinAlgTests, Vec3BasicTest) {
    using namespace csics::linalg;

    Vec3<double> v1{1.0, 2.0, 3.0};
    Vec3<double> v2{4.0, 5.0, 6.0};

    ASSERT_NE(v1, v2);
    auto v3 = v1 + v2;
    Vec3<double> expected{5.0, 7.0, 9.0};
    ASSERT_EQ(v3, expected); 
    auto v4 = v2 - v1;
    Vec3<double> expected_sub{3.0, 3.0, 3.0};
    ASSERT_EQ(v4, expected_sub);

    auto v5 = v1 * 2.0;
    Vec3<double> expected_mul{2.0, 4.0, 6.0};
    ASSERT_EQ(v5, expected_mul);
    auto v6 = 2.0 * v1;
    ASSERT_EQ(v6, expected_mul);
    auto v7 = v1 / 2.0;
    Vec3<double> expected_div{0.5, 1.0, 1.5};
    ASSERT_EQ(v7, expected_div);

};

TEST(CSICSLinAlgTests, Vec4BasicTest) {
    using namespace csics::linalg;

    Vec4<double> v1{1.0, 2.0, 3.0, 4.0};
    Vec4<double> v2{5.0, 6.0, 7.0, 8.0};

    ASSERT_NE(v1, v2);
    auto v3 = v1 + v2;
    Vec4<double> expected{6.0, 8.0, 10.0, 12.0};
    ASSERT_EQ(v3, expected); 
    auto v4 = v2 - v1;
    Vec4<double> expected_sub{4.0, 4.0, 4.0, 4.0};
    ASSERT_EQ(v4, expected_sub);

    auto v5 = v1 * 2.0;
    Vec4<double> expected_mul{2.0, 4.0, 6.0, 8.0};
    ASSERT_EQ(v5, expected_mul);
    auto v6 = 2.0 * v1;
    ASSERT_EQ(v6, expected_mul);
    auto v7 = v1 / 2.0;
    Vec4<double> expected_div{0.5, 1.0, 1.5, 2.0};
    ASSERT_EQ(v7, expected_div);
};



