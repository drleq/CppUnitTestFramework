#include "CppUnitTestFramework.hpp"

using namespace CppUnitTestFramework;

namespace {
    struct AssertTest {};

    struct BoolWrapper {
        explicit BoolWrapper(bool value)
          : Value(value)
        {}

        const bool Value;
        explicit operator bool() const { return Value; }
    };
}

namespace CppUnitTestFrameworkTest {

    TEST_CASE(AssertTest, AreEqual_Integer) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_EQUAL(0, 0));
            CHECK_NO_THROW(REQUIRE_EQUAL(10, 10));
            CHECK_NO_THROW(REQUIRE_EQUAL(-10, -10));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE_EQUAL(0, 1));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(1, 0));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(0, -1));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(-1, 0));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, AreEqual_String) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_EQUAL("", ""));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string(), ""));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string_view(), ""));
            CHECK_NO_THROW(REQUIRE_EQUAL("", std::string()));
            CHECK_NO_THROW(REQUIRE_EQUAL("", std::string_view()));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string(), std::string_view()));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string_view(), std::string()));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string(), std::string()));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string_view(), std::string_view()));

            CHECK_NO_THROW(REQUIRE_EQUAL("Value", "Value"));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string("Value"), "Value"));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string_view("Value"), "Value"));
            CHECK_NO_THROW(REQUIRE_EQUAL("Value", std::string("Value")));
            CHECK_NO_THROW(REQUIRE_EQUAL("Value", std::string_view("Value")));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string("Value"), std::string_view("Value")));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string_view("Value"), std::string("Value")));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string("Value"), std::string("Value")));
            CHECK_NO_THROW(REQUIRE_EQUAL(std::string_view("Value"), std::string_view("Value")));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE_EQUAL("left", ""));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string("left"), ""));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string_view("left"), ""));
            CHECK_THROW(AssertException, REQUIRE_EQUAL("left", std::string()));
            CHECK_THROW(AssertException, REQUIRE_EQUAL("left", std::string_view()));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string("left"), std::string_view()));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string_view("left"), std::string()));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string("left"), std::string()));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string_view("left"), std::string_view()));

            CHECK_THROW(AssertException, REQUIRE_EQUAL("left", "Value"));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string("left"), "Value"));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string_view("left"), "Value"));
            CHECK_THROW(AssertException, REQUIRE_EQUAL("left", std::string("Value")));
            CHECK_THROW(AssertException, REQUIRE_EQUAL("left", std::string_view("Value")));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string("left"), std::string_view("Value")));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string_view("left"), std::string("Value")));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string("left"), std::string("Value")));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(std::string_view("left"), std::string_view("Value")));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, AreEqual_Boolean) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_EQUAL(true, true));
            CHECK_NO_THROW(REQUIRE_EQUAL(false, false));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE_EQUAL(true, false));
            CHECK_THROW(AssertException, REQUIRE_EQUAL(false, true));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, IsNull) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_NULL(nullptr));
            CHECK_NO_THROW(REQUIRE_NULL(reinterpret_cast<int*>(0)));
        }

        SECTION("Check fails") {
            int v;
            CHECK_THROW(AssertException, REQUIRE_NULL(reinterpret_cast<void*>(&v)));
            CHECK_THROW(AssertException, REQUIRE_NULL(&v));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, IsNotNull) {
        SECTION("Check passes") {
            int v;
            CHECK_NO_THROW(REQUIRE_NOT_NULL(reinterpret_cast<void*>(&v)));
            CHECK_NO_THROW(REQUIRE_NOT_NULL(&v));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE_NOT_NULL(nullptr));
            CHECK_THROW(AssertException, REQUIRE_NOT_NULL(reinterpret_cast<int*>(0)));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, IsTrue) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE(true));
            CHECK_NO_THROW(REQUIRE_TRUE(true));

            CHECK_NO_THROW(REQUIRE(BoolWrapper(true)));
            CHECK_NO_THROW(REQUIRE_TRUE(BoolWrapper(true)));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE(false));
            CHECK_THROW(AssertException, REQUIRE_TRUE(false));

            CHECK_THROW(AssertException, REQUIRE(BoolWrapper(false)));
            CHECK_THROW(AssertException, REQUIRE_TRUE(BoolWrapper(false)));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, IsFalse) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_FALSE(false));
            CHECK_NO_THROW(REQUIRE_FALSE(BoolWrapper(false)));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE_FALSE(true));
            CHECK_THROW(AssertException, REQUIRE_FALSE(BoolWrapper(true)));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, Throws) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_THROW(std::runtime_error, throw std::runtime_error("Bang")));
            CHECK_NO_THROW(REQUIRE_THROW(int, throw 10));
        }

        SECTION("Check fails") {
            CHECK_THROW(
                AssertException,
                REQUIRE_THROW(std::runtime_error, throw std::logic_error("Bang"))
            );
            CHECK_THROW(
                AssertException,
                REQUIRE_THROW(std::runtime_error, /*nothing*/)
            );
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, NoThrow) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_NO_THROW(return /*nothing*/));
        }

        SECTION("Check fails") {
            CHECK_THROW(
                AssertException,
                REQUIRE_NO_THROW(throw std::logic_error("Bang"))
            );
            CHECK_THROW(
                AssertException,
                REQUIRE_NO_THROW(throw 10)
            );
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, Close_Float) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_CLOSE(10.0f, 10.0f, 0.0f));

            CHECK_NO_THROW(REQUIRE_CLOSE(10.0f, 11.0f, 0.10f));
            CHECK_NO_THROW(REQUIRE_CLOSE(11.0f, 10.0f, 0.10f));
            CHECK_NO_THROW(REQUIRE_CLOSE(10.0f, 10.1f, 0.011f));
            CHECK_NO_THROW(REQUIRE_CLOSE(10.1f, 10.0f, 0.011f));

            CHECK_NO_THROW(REQUIRE_CLOSE(-10.0f, -11.0f, 0.10f));
            CHECK_NO_THROW(REQUIRE_CLOSE(-11.0f, -10.0f, 0.10f));
            CHECK_NO_THROW(REQUIRE_CLOSE(-10.0f, -10.1f, 0.011f));
            CHECK_NO_THROW(REQUIRE_CLOSE(-10.1f, -10.0f, 0.011f));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.0f, 11.0f, 0.0f));

            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.0f, 11.0f, 0.09f));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(11.0f, 10.0f, 0.09f));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.0f, 10.1f, 0.009f));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.1f, 10.0f, 0.009f));

            CHECK_THROW(AssertException, REQUIRE_CLOSE(-10.0f, -11.0f, 0.09f));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(-11.0f, -10.0f, 0.09f));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(-10.0f, -10.1f, 0.009f));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(-10.1f, -10.0f, 0.009f));
        }
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE(AssertTest, Close_Double) {
        SECTION("Check passes") {
            CHECK_NO_THROW(REQUIRE_CLOSE(10.0, 10.0, 0.0));

            CHECK_NO_THROW(REQUIRE_CLOSE(10.0, 11.0, 0.10));
            CHECK_NO_THROW(REQUIRE_CLOSE(11.0, 10.0, 0.10));
            CHECK_NO_THROW(REQUIRE_CLOSE(10.0, 10.1, 0.011));
            CHECK_NO_THROW(REQUIRE_CLOSE(10.1, 10.0, 0.011));

            CHECK_NO_THROW(REQUIRE_CLOSE(-10.0, -11.0, 0.10));
            CHECK_NO_THROW(REQUIRE_CLOSE(-11.0, -10.0, 0.10));
            CHECK_NO_THROW(REQUIRE_CLOSE(-10.0, -10.1, 0.011));
            CHECK_NO_THROW(REQUIRE_CLOSE(-10.1, -10.0, 0.011));
        }

        SECTION("Check fails") {
            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.0, 11.0, 0.0));

            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.0, 11.0, 0.09));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(11.0, 10.0, 0.09));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.0, 10.1, 0.009));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(10.1, 10.0, 0.009));

            CHECK_THROW(AssertException, REQUIRE_CLOSE(-10.0, -11.0, 0.09));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(-11.0, -10.0, 0.09));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(-10.0, -10.1, 0.009));
            CHECK_THROW(AssertException, REQUIRE_CLOSE(-10.1, -10.0, 0.009));
        }
    }

}