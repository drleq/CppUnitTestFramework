#include "CppUnitTestFramework.hpp"

using namespace CppUnitTestFramework;

namespace {
    struct TestLogger : ILogger
    {
    public:
        virtual ~TestLogger() = default;

        void AttachLogger(const ILoggerPtr& logger) {
            m_real_logger = logger;
        }

        std::string GetLogOutput() const {
            return m_section_log.str();
        }
        void ClearLogOutput() {
            m_section_log = std::ostringstream();
        }

    public:
        void BeginRun(size_t /*test_count*/) override {
            throw std::runtime_error("Unexpected function called");
        }
        void EndRun(size_t /*pass_count*/, size_t /*fail_count*/, size_t /*skip_count*/) override {
            throw std::runtime_error("Unexpected function called");
        }

        void SkipTest(const std::string_view& /*name*/) override {
            throw std::runtime_error("Unexpected function called");
        }
        void EnterTest(const std::string_view& /*name*/) override {
            throw std::runtime_error("Unexpected function called");
        }
        void ExitTest(bool /*failed*/) override {
            throw std::runtime_error("Unexpected function called");
        }

        void PushSection(const std::string_view& name) override {
            m_section_log << "Push " << name << std::endl;
            m_real_logger->PushSection(name);
        }
        void PopSection() override {
            m_section_log << "Pop" << std::endl;
            m_real_logger->PopSection();
        }

        void AssertFailed(
            AssertType type,
            const AssertLocation& location,
            const std::string_view& message
        ) override {
            m_real_logger->AssertFailed(type, location, message);
        }
        void UnhandledException(const std::string_view& message) override {
            m_real_logger->UnhandledException(message);
        }

    private:
        std::ostringstream m_section_log;
        ILoggerPtr m_real_logger;
    };

    //--------------------------------------------------------------------------------------------------------

    struct SectionTest : CommonFixture {
        SectionTest(const ILoggerPtr& logger)
          : CommonFixture(GetTestLogger())
        {
            GetTestLogger()->AttachLogger(logger);
            GetTestLogger()->ClearLogOutput();
        }

        virtual void Run() = 0;

    protected:
        std::string GetTestLog() const {
            return GetTestLogger()->GetLogOutput();
        }

    private:
        static const std::shared_ptr<TestLogger>& GetTestLogger() {
            static const auto s_test_logger = std::make_shared<TestLogger>();
            return s_test_logger;
        }
    };
}

namespace CppUnitTestFrameworkTest {

    namespace {
        // We need a custom base type to hook the logger API.  This means manually rolling the test case.
        struct TestCase_Nesting : SectionTest {
            using SectionTest::SectionTest;
            static constexpr std::string_view SourceFile = __FILE__;
            static constexpr size_t SourceLine = __LINE__;
            static constexpr std::string_view Name = "SectionTest::Nesting";
            static constexpr auto Tags = make_tags_array();

            void Run() override {
                CHECK_EQUAL(GetTestLog(), "");

                SECTION("Outer") {
                    CHECK_EQUAL(GetTestLog(), "Push Section: Outer\n");
                    SECTION("Inner") {
                        CHECK_EQUAL(GetTestLog(), "Push Section: Outer\nPush Section: Inner\n");
                    }
                    CHECK_EQUAL(GetTestLog(), "Push Section: Outer\nPush Section: Inner\nPop\n");
                }

                CHECK_EQUAL(GetTestLog(), "Push Section: Outer\nPush Section: Inner\nPop\nPop\n");
            }
        };
        TestRegistry::AutoReg<TestCase_Nesting> s_test_registrar_Nesting;
    }

    //--------------------------------------------------------------------------------------------------------

    namespace {
        // We need a custom base type to hook the logger API.  This means manually rolling the test case.
        struct TestCase_BDD : SectionTest {
            using SectionTest::SectionTest;
            static constexpr std::string_view SourceFile = __FILE__;
            static constexpr size_t SourceLine = __LINE__;
            static constexpr std::string_view Name = "SectionTest::BDD";
            static constexpr auto Tags = make_tags_array();

            void Run() override {
                CHECK_EQUAL(GetTestLog(), "");

                SCENARIO("Refunded items should be returned to stock") {
                    GIVEN("a customer previously bought a black sweater from me") {}
                    AND("I have three black sweaters in stock") {}
                    WHEN("they return the black sweater for a refund") {}
                    THEN("I should have four black sweaters in stock") {}
                }

                CHECK_EQUAL(
                    GetTestLog(),
                    R"^^(Push Scenario: Refunded items should be returned to stock
Push Given: a customer previously bought a black sweater from me
Pop
Push And: I have three black sweaters in stock
Pop
Push When: they return the black sweater for a refund
Pop
Push Then: I should have four black sweaters in stock
Pop
Pop
)^^"
                );
            }
        };
        TestRegistry::AutoReg<TestCase_BDD> s_test_registrar_BDD;
    }

}