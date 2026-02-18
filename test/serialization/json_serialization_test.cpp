#include <gtest/gtest.h>

#include <csics/serialization/JSONSerializer.hpp>

class TestClass {
   private:
    int a;
    double b;
    std::string c;

   public:
    TestClass(int a, double b, std::string c) : a(a), b(b), c(c) {}
    static consteval auto fields() {
        using namespace csics::serialization;
        return make_fields(make_field("a", &TestClass::a),
                                     make_field("b", &TestClass::b),
                                     make_field("c", &TestClass::c));
    }
};

class TestClass2 {
    private:
        TestClass nested;
        int d;
    public:
        TestClass2(TestClass nested, int d) : nested(nested), d(d) {}
        static consteval auto fields() {
            using namespace csics::serialization;
            return make_fields(make_field("nested", &TestClass2::nested),
                               make_field("d", &TestClass2::d));
        }
};

TEST(CSICSSerializationTests, JSONStructSerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    TestClass c(42, 3.14, "Hello, world!");

    constexpr std::string_view expected =
        R"({"a":42,"b":3.14,"c":"Hello, world!"})";

    csics::io::BufferView bv(buffer, sizeof(buffer));
    auto out = serialize(serializer, bv, c);
    ASSERT_EQ(out.status, SerializationStatus::Ok);
    std::string_view result(out.written_view.data(), out.written_view.size());
    EXPECT_EQ(result, expected);
};

TEST(CSICSSerializationTests, JSONArraySerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    std::vector<int> vec = {1, 2, 3, 4, 5};

    constexpr std::string_view expected = R"([1,2,3,4,5])";

    csics::io::BufferView bv(buffer, sizeof(buffer));
    auto res = serialize(serializer, bv, vec);
    ASSERT_EQ(res.status, SerializationStatus::Ok);
    std::string_view result(res.written_view.data(), res.written_view.size());
    EXPECT_EQ(result, expected);
};

TEST(CSICSSerializationTests, JSONNestedStructSerialization) {

    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    TestClass c(42, 3.14, "Hello, world!");
    TestClass2 c2(c, 99);

    constexpr std::string_view expected =
        R"({"nested":{"a":42,"b":3.14,"c":"Hello, world!"},"d":99})";

    csics::io::BufferView bv(buffer, sizeof(buffer));
    auto out = serialize(serializer, bv, c2);
    ASSERT_EQ(out.status, SerializationStatus::Ok);
    std::string_view result(out.written_view.data(), out.written_view.size());
    EXPECT_EQ(result, expected);
}
