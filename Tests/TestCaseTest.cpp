#include "CppUnitTestFramework.hpp"

namespace {
    struct TestCaseTest {};
}

namespace CppUnitTestFrameworkTest {

    TEST_CASE(TestCaseTest, TestWithoutTags) {
        CHECK_EQUAL(SourceFile, __FILE__);
        CHECK_EQUAL(SourceLine, __LINE__ - 2);
        CHECK_EQUAL(Name, "TestCaseTest::TestWithoutTags");
        CHECK(Tags == make_tags_array());
    }

    //--------------------------------------------------------------------------------------------------------

    TEST_CASE_WITH_TAGS(TestCaseTest, TestWithTags, "Tag1", "Tag2") {
        CHECK_EQUAL(SourceFile, __FILE__);
        CHECK_EQUAL(SourceLine, __LINE__ - 2);
        CHECK_EQUAL(Name, "TestCaseTest::TestWithTags");
        CHECK(Tags == make_tags_array("Tag1", "Tag2"));
    }

}