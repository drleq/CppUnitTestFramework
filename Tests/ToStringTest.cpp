#include "CppUnitTestFramework.hpp"

using namespace CppUnitTestFramework;

namespace {
    struct ToStringTest {
        enum Untyped { Value1 = 10, Value2 = -20 };
        enum class Typed : uint8_t {
            Value1 = 10,
            Value2 = 200
        };
    };

    struct CustomType {
        int Value;
    };
}

namespace CppUnitTestFramework::Ext {
    std::string ToString(const CustomType& value) {
        std::ostringstream ss;
        ss << "[CustomType] " << value.Value;
        return ss.str();
    }
}

namespace CppUnitTestFrameworkTest {

    TEST_CASE(ToStringTest, Nullptr) {
        CHECK_EQUAL(Ext::ToString(nullptr), "nullptr");
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(ToStringTest, Pointer) {
        const void* zero_ptr = 0;
        const int* int_ptr = reinterpret_cast<int*>(0x123456);
        const float* float_ptr = reinterpret_cast<float*>(0x123456789abcdef);

        CHECK_EQUAL(Ext::ToString(zero_ptr), "0x0000000000000000");
        CHECK_EQUAL(Ext::ToString(int_ptr), "0x0000000000123456");
        CHECK_EQUAL(Ext::ToString(float_ptr), "0x0123456789abcdef");
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(ToStringTest, Enum) {
        CHECK_EQUAL(Ext::ToString(Untyped::Value1), "[enum `anonymous namespace'::ToStringTest::Untyped] 10");
        CHECK_EQUAL(Ext::ToString(Untyped::Value2), "[enum `anonymous namespace'::ToStringTest::Untyped] -20");
        CHECK_EQUAL(Ext::ToString(Typed::Value1), "[enum `anonymous namespace'::ToStringTest::Typed] 10");
        CHECK_EQUAL(Ext::ToString(Typed::Value2), "[enum `anonymous namespace'::ToStringTest::Typed] 200");
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(ToStringTest, FloatingPoint) {
        SECTION("float") {
            CHECK_EQUAL(Ext::ToString(1.0f), "1");
            CHECK_EQUAL(Ext::ToString(1.5f), "1.5");
            CHECK_EQUAL(Ext::ToString(1e-10f), "1e-10");
            CHECK_EQUAL(Ext::ToString(-1.234f), "-1.234");
        }

        SECTION("double") {
            CHECK_EQUAL(Ext::ToString(1.0), "1");
            CHECK_EQUAL(Ext::ToString(1.5), "1.5");
            CHECK_EQUAL(Ext::ToString(1e-10), "1e-10");
            CHECK_EQUAL(Ext::ToString(-1.234), "-1.234");
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(ToStringTest, Integers) {
        CHECK_EQUAL(Ext::ToString(static_cast<int8_t>(-123)), "-123");
        CHECK_EQUAL(Ext::ToString(static_cast<int16_t>(-1234)), "-1234");
        CHECK_EQUAL(Ext::ToString(static_cast<int32_t>(-123456)), "-123456");
        CHECK_EQUAL(Ext::ToString(static_cast<int64_t>(-123456)), "-123456");

        CHECK_EQUAL(Ext::ToString(static_cast<uint8_t>(123)), "123");
        CHECK_EQUAL(Ext::ToString(static_cast<uint16_t>(1234)), "1234");
        CHECK_EQUAL(Ext::ToString(static_cast<uint32_t>(123456)), "123456");
        CHECK_EQUAL(Ext::ToString(static_cast<uint64_t>(123456)), "123456");
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(ToStringTest, Strings) {
        CHECK_EQUAL(Ext::ToString("Hello world"), "Hello world");
        CHECK_EQUAL(Ext::ToString(std::string("Hello world")), "Hello world");
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(ToStringTest, Nullopt) {
        CHECK_EQUAL(Ext::ToString(std::nullopt), "?");
        CHECK_EQUAL(Ext::ToString(std::optional<int>()), "?");
        CHECK_EQUAL(Ext::ToString(std::make_optional(10)), "10");
        CHECK_EQUAL(Ext::ToString(std::make_optional("Hello world")), "Hello world");
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(ToStringTest, CustomType) {
        CustomType value { 1234 };
        CHECK_EQUAL(Ext::ToString(value), "[CustomType] 1234");
    }

}