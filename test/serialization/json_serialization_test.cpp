#include <gtest/gtest.h>

#include <csics/csics.hpp>

class TestClass {
   private:
    int a;
    double b;
    std::string c;
    std::vector<int> d;
    std::vector<int> e;

   public:
    TestClass(int a, double b, std::string c, std::vector<int> d,
              std::vector<int> e)
        : a(a), b(b), c(c), d(d), e(e) {}

    static consteval auto fields() {
        using namespace csics::serialization;
        return make_fields(
            make_field("a", &TestClass::a), make_field("b", &TestClass::b),
            make_field("c", &TestClass::c), make_field("d", &TestClass::d),
            make_field("e", &TestClass::e));
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

class EmptyClass {
   public:
    static consteval auto fields() { return std::tuple{}; }
};

TEST(CSICSSerializationTests, JSONStructSerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    TestClass c(42, 3.14, "Hello, world!", {1, 2, 3}, {4, 5, 6});

    constexpr std::string_view expected =
        R"({"a":42,"b":3.14,"c":"Hello, world!","d":[1,2,3],"e":[4,5,6]})";

    csics::MutableBufferView bv(buffer, sizeof(buffer));
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

    csics::MutableBufferView bv(buffer, sizeof(buffer));
    auto res = serialize(serializer, bv, vec);
    ASSERT_EQ(res.status, SerializationStatus::Ok);
    std::string_view result(res.written_view.data(), res.written_view.size());
    EXPECT_EQ(result, expected);
};

TEST(CSICSSerializationTests, JSONNestedStructSerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    TestClass c(42, 3.14, "Hello, world!", {1, 2, 3}, {4, 5, 6});
    TestClass2 c2(c, 99);

    constexpr std::string_view expected =
        R"({"nested":{"a":42,"b":3.14,"c":"Hello, world!","d":[1,2,3],"e":[4,5,6]},"d":99})";

    csics::MutableBufferView bv(buffer, sizeof(buffer));
    auto out = serialize(serializer, bv, c2);
    ASSERT_EQ(out.status, SerializationStatus::Ok);
    std::string_view result(out.written_view.data(), out.written_view.size());
    EXPECT_EQ(result, expected);
}

TEST(CSICSSerializationTests, JSONEmptyStructSerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    EmptyClass c;

    constexpr std::string_view expected = R"({})";

    csics::MutableBufferView bv(buffer, sizeof(buffer));
    auto out = serialize(serializer, bv, c);
    ASSERT_EQ(out.status, SerializationStatus::Ok);
    std::string_view result(out.written_view.data(), out.written_view.size());
    EXPECT_EQ(result, expected);
}

TEST(CSICSSerializationTests, JSONEmptyArraySerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    std::vector<int> vec;

    constexpr std::string_view expected = R"([])";

    csics::MutableBufferView bv(buffer, sizeof(buffer));
    auto res = serialize(serializer, bv, vec);
    ASSERT_EQ(res.status, SerializationStatus::Ok);
    std::string_view result(res.written_view.data(), res.written_view.size());
    EXPECT_EQ(result, expected);
}

TEST(CSICSSerializationTests, JSONArrayOfStructsSerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    std::vector<TestClass> vec = {
        TestClass(1, 1.1, "One", {1}, {2}),
        TestClass(2, 2.2, "Two", {3}, {4}),
        TestClass(3, 3.3, "Three", {5}, {6}),
    };

    constexpr std::string_view expected =
        R"([{"a":1,"b":1.1,"c":"One","d":[1],"e":[2]},{"a":2,"b":2.2,"c":"Two","d":[3],"e":[4]},{"a":3,"b":3.3,"c":"Three","d":[5],"e":[6]}])";

    csics::MutableBufferView bv(buffer, sizeof(buffer));
    auto res = serialize(serializer, bv, vec);
    ASSERT_EQ(res.status, SerializationStatus::Ok);
    std::string_view result(res.written_view.data(), res.written_view.size());
    EXPECT_EQ(result, expected);
}

TEST(CSICSSerializationTests, JSONMapSerialization) {
    using namespace csics::serialization;

    JSONSerializer serializer;
    char buffer[256];
    std::map<std::string, int> m = {{"one", 1}, {"two", 2}, {"three", 3}};

    constexpr std::string_view expected =
        R"({"one":1,"three":3,"two":2})";  // Note: std::map orders keys

    csics::MutableBufferView bv(buffer, sizeof(buffer));
    auto res = serialize(serializer, bv, m);
    ASSERT_EQ(res.status, SerializationStatus::Ok);
    std::string_view result(res.written_view.data(), res.written_view.size());
    EXPECT_EQ(result, expected);
}
