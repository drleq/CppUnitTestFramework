#pragma once

#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace CppUnitTestFramework {

    struct AssertLocation {
        std::string_view SourceFile;
        size_t LineNumber;
    };

    enum class AssertType {
        Throw,
        Continue
    };

    struct AssertException :
        std::exception
    {
        AssertException(std::string message)
          : m_message(std::move(message))
        {}

        const char* what() const noexcept override {
            return m_message.data();
        }

    private:
        std::string m_message;
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------

    struct RunOptions {
        bool Verbose = false;
        std::vector<std::string> Keywords;

        bool ParseCommandLine(int argc, const char* argv[]) {
            for (int index = 1; index < argc; ++index) {
                auto arg = argv[index];

                if (arg[0] != '-') {
                    Keywords.push_back(arg);
                    continue;
                }

                std::string option_name{ &arg[1] };
                if (option_name == "h" || option_name == "-help" || option_name == "?") {
                    std::cout << "Usage:" << std::endl;
                    std::cout << "    -h, --help, -?:  Displays this message" << std::endl;
                    std::cout << "    -v, --verbose:   Show verbose output" << std::endl;
                    return false;
                }

                if (option_name == "v" || option_name == "-verbose") {
                    Verbose = true;
                    continue;
                }

                // Unknown option
                std::cout << "Unknown option: " << option_name << std::endl;
                return false;
            }

            return true;
        }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------

    struct ILogger {
        virtual ~ILogger() = default;

        virtual void BeginRun(size_t test_count) = 0;
        virtual void EndRun(size_t pass_count, size_t fail_count, size_t skip_count) = 0;

        virtual void SkipTest(const std::string_view& name) = 0;
        virtual void EnterTest(const std::string_view& name) = 0;
        virtual void ExitTest() = 0;

        virtual void SkipSection(const std::string_view& name) = 0;
        virtual void PushSection(const std::string_view& name) = 0;
        virtual void PopSection() = 0;

        virtual void AssertFailed(
            AssertType type,
            const AssertLocation& location,
            const std::string_view& message
        ) = 0;
        virtual void UnhandledException(const std::string_view& message) = 0;
    };
    using ILoggerPtr = std::shared_ptr<ILogger>;

    //--------------------------------------------------------------------------------------------------------

    struct ConsoleLogger :
        ILogger
    {
        static ILoggerPtr Create(const RunOptions* options) {
            return std::unique_ptr<ConsoleLogger>{ new ConsoleLogger(options) };
        }

        virtual ~ConsoleLogger() = default;

        void BeginRun(size_t test_count) override {
            std::cout << "Running " << test_count << " test cases..." << std::endl;
        }
        void EndRun(size_t pass_count, size_t fail_count, size_t skip_count) override {
            std::cout << "Complete." << std::endl;
            std::cout << "    Passed:  " << pass_count << std::endl;
            std::cout << "    Failed:  " << fail_count << std::endl;
            std::cout << "    Skipped: " << skip_count << std::endl;
        }

        void SkipTest(const std::string_view& name) override {
            m_test_log.clear();
            m_test_log << "Skip: " << name.data() << std::endl;
            
            if (m_run_options->Verbose) {
                FlushLog();
            }
        }
        void EnterTest(const std::string_view& name) override {
            m_test_failed = false;
            m_test_log = std::stringstream();
            m_test_log << "Test: " << name.data() << std::endl;
            m_indent_level++;

            if (m_run_options->Verbose) {
                FlushLog();
            }
        }
        void ExitTest() override {
            m_indent_level = 0;

            if (m_test_failed || m_run_options->Verbose) {
                FlushLog();
            }
        }

        void SkipSection(const std::string_view& name) override {
            Indent() << "[Skipped] " << name.data() << std::endl;

            if (m_run_options->Verbose) {
                FlushLog();
            }
        }
        void PushSection(const std::string_view& name) override {
            Indent() << name.data() << std::endl;
            m_indent_level++;

            if (m_run_options->Verbose) {
                FlushLog();
            }
        }
        void PopSection() override {
            if (m_indent_level > 0) {
                m_indent_level--;
            }
        }

        void AssertFailed(
            AssertType type,
            const AssertLocation& location,
            const std::string_view& message
        ) override {
            m_test_failed = true;
            auto& log = Indent();

            log << "@" << location.LineNumber << " ";

            switch (type) {
            case AssertType::Throw: log << "REQUIRE"; break;
            case AssertType::Continue: log << "CHECK"; break;
            }

            log << ": " << message.data() << std::endl;
            
            if (m_run_options->Verbose) {
                FlushLog();
            }
        }
        void UnhandledException(const std::string_view& message) override {
            m_test_failed = true;
            Indent() << "Fail: " << message.data() << std::endl;

            if (m_run_options->Verbose) {
                FlushLog();
            }
        }

    private:
        ConsoleLogger(const RunOptions* run_options)
          : m_run_options(run_options)
        {}

        std::ostream& Indent() {
            for (size_t i = 0; i != m_indent_level; ++i) {
                m_test_log << "    ";
            }
            return m_test_log;
        }

        void FlushLog() {
            std::cout << m_test_log.str().c_str();
            m_test_log = std::stringstream();
        }

    private:
        const RunOptions*const m_run_options;

        bool m_test_failed = false;
        size_t m_indent_level = 0;
        std::stringstream m_test_log;
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------

    struct TestRegistry {
    private:
        using TestCallback = std::function<bool (const ILoggerPtr& logger)>;
        struct TestDetails {
            std::string_view Name;
            std::vector<std::string_view> Tags;
            TestCallback Callback;
        };

    public:
        template <typename TTestCase>
        struct AutoReg {
            AutoReg() {
                TestRegistry::Add<TTestCase>();
            }
        };

    public:
        template <typename TTestCase>
        static void Add() {
            TestDetails details;
            details.Name = TTestCase::Name;
            details.Tags.assign(std::begin(TTestCase::Tags), std::end(TTestCase::Tags));
            details.Callback = [](const auto&... args) -> bool {
                TTestCase test_case(args...);
                test_case.Run();
                return test_case.HaveChecksFailed();
            };

            GetTestVector().push_back(std::move(details));
        }

        static void Run(const RunOptions* options, const ILoggerPtr& logger) {
            const auto& all_test_cases = GetTestVector();

            logger->BeginRun(all_test_cases.size());

            size_t pass_count = 0;
            size_t fail_count = 0;
            size_t skip_count = 0;

            for (auto& test_case : all_test_cases) {
                if (!ShouldRunTest(options, test_case.Name, test_case.Tags)) {
                    logger->SkipTest(test_case.Name);
                    skip_count++;
                    continue;
                }

                logger->EnterTest(test_case.Name);

                bool test_failed = true;
                try {
                    test_failed = test_case.Callback(logger);
                } catch (const AssertException& e) {
                    // REQUIRE* statement failed.  No need to do anything else.
                } catch (const std::exception& e) {
                    logger->UnhandledException(e.what());
                } catch (...) {
                    logger->UnhandledException("<unstructured>");
                }

                logger->ExitTest();

                if (test_failed) {
                    fail_count++;
                } else {
                    pass_count++;
                }
            }

            logger->EndRun(pass_count, fail_count, skip_count);
        }

    private:
        static std::vector<TestDetails>& GetTestVector() {
            static std::vector<TestDetails> s_test_vector;
            return s_test_vector;
        }

        static bool ShouldRunTest(
            const RunOptions* options,
            const std::string_view& test_name,
            const std::vector<std::string_view> test_tags
        ) {
            if (options->Keywords.empty()) {
                // No keywords.  All tests match.
                return true;
            }

            for (auto& keyword : options->Keywords) {
                if (test_name.find(keyword) != std::string_view::npos) {
                    return true;
                }

                for (auto& tag : test_tags) {
                    if (tag == keyword) {
                        return true;
                    }
                }
            }

            return false;
        }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------

    namespace Ext {
        template <typename T>
        std::string ToString([[maybe_unused]] const T& value) {
            if constexpr (std::is_null_pointer_v<T>) {
                // std::nullptr_t
                return "nullptr";

            } else if constexpr (std::is_constructible_v<std::string, const T&>) {
                // std::string(const T&)
                return std::string(value);

            } else if constexpr (std::is_pointer_v<T>) {
                // <pointer> -> Hex address
                std::ostringstream ss;
                ss << "0x" << std::hex << std::setfill('0') << std::setw(sizeof(size_t) * 2)
                    << reinterpret_cast<std::uintptr_t>(value);
                return ss.str();

            } else if constexpr (std::is_enum_v<T>) {
                // Enum -> [<name>] <number>
                std::ostringstream ss;
                ss << "[" << typeid(T).name() << "] " << static_cast<std::underlying_type_t<T>>(value);
                return ss.str();

            } else {
                // std::to_string(const T&)
                return std::to_string(value);
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------

    namespace Assert {
        template <typename TLeft, typename TRight>
        std::optional<AssertException> AreEqual(TLeft&& left, TRight&& right) {
            bool equal = (left == right);
            if (equal) {
                return std::nullopt;
            }

            auto msg = "[" + Ext::ToString(left) + "] == [" + Ext::ToString(right) + "]";
            return AssertException(msg.c_str());
        }

        //----------------------------------------------------------------------------------------------------

        inline std::optional<AssertException> AreEqual(const char* left, const char* right) {
            bool equal = (std::strcmp(left, right) == 0);
            if (equal) {
                return std::nullopt;
            }

            auto msg = "[" + Ext::ToString(left) + "] == [" + Ext::ToString(right) + "]";
            return AssertException(msg.c_str());
        }

        //----------------------------------------------------------------------------------------------------

        template <typename T>
        std::optional<AssertException> IsNull(const T& value, const char* expression) {
            if (value == nullptr) {
                return std::nullopt;
            }

            std::ostringstream ss;
            ss << "IsNull(" << expression << ")";
            return AssertException(ss.str());
        }

        //----------------------------------------------------------------------------------------------------

        inline std::optional<AssertException> IsTrue(bool value, const char* expression) {
            if (!value) {
                std::ostringstream ss;
                ss << "IsTrue(" << expression << ")";
                return AssertException(ss.str());
            }
            return std::nullopt;
        }

        //----------------------------------------------------------------------------------------------------

        inline std::optional<AssertException> IsFalse(bool value, const char* expression) {
            if (value) {
                std::ostringstream ss;
                ss << "IsFalse(" << expression << ")";
                return AssertException(ss.str());
            }
            return std::nullopt;
        }

        //----------------------------------------------------------------------------------------------------

        template <typename TException, typename Callback>
        inline std::optional<AssertException> Throws(const Callback& callback) {
            try {
                callback();
            } catch (const TException&) {
                return std::nullopt;
            } catch (const std::exception& e) {
                std::ostringstream ss;
                ss << "Expected exception [" << typeid(TException).name() << "] but caught ["
                    << typeid(e).name() << ": " << e.what() << "]";
                return AssertException(ss.str().c_str());
            } catch (...) {
                std::ostringstream ss;
                ss << "Expected exception [" << typeid(TException).name() << "] but caught another";
                return AssertException(ss.str().c_str());
            }

            std::ostringstream ss;
            ss << "Expected exception [" << typeid(TException).name() << "] but none was thrown";
            return AssertException(ss.str().c_str());
        }
    };

    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------------------------

    struct CommonFixture;
    struct SectionLock {
        ~SectionLock() {
            if (m_logger00) {
                m_logger00->PopSection();
                m_logger00 = nullptr;
            }
        }

    private:
        SectionLock(const std::string_view& text, ILoggerPtr logger)
          : m_logger00(std::move(logger))
        {
            m_logger00->PushSection(text);
        }

        SectionLock(SectionLock&& other)
          : m_logger00(std::move(other.m_logger00))
        {}

        SectionLock& operator = (SectionLock&& other) {
            if (m_logger00) {
                m_logger00->PopSection();
            }
            m_logger00 = std::move(other.m_logger00);
            return *this;
        }

        ILoggerPtr m_logger00;

        friend struct CommonFixture;
    };

    //--------------------------------------------------------------------------------------------------------

    struct CommonFixture
    {
        CommonFixture(ILoggerPtr logger)
          : m_logger(std::move(logger))
        {}

        bool HaveChecksFailed() const {
            return m_check_has_failed;
        }

    protected:
        SectionLock EnterSection(std::string text) {
            return SectionLock(text, m_logger);
        }

        void HandleAssert(
            AssertType behavior,
            const AssertLocation& location,
            const std::optional<AssertException>& exception00
        ) {
            if (!exception00) {
                // Nothing happened.
                return;
            }

            m_logger->AssertFailed(behavior, location, exception00->what());

            if (behavior == AssertType::Throw) {
                throw *exception00;
            } else {
                m_check_has_failed = true;
            }
        }

        // In leiu of std::make_array()
        template <typename... TArgs>
        static constexpr auto make_tags_array(TArgs&&... tags) {
            return std::array<std::string_view, sizeof...(TArgs)> {
                std::forward<TArgs>(tags)...
            };
        }

    private:
        bool m_check_has_failed = false;
        ILoggerPtr m_logger;
    };
}

//------------------------------------------------------------------------------------------------------------

#define _CPPUTF_COMBINE_NAME_PARTS2(a, b) a##b
#define _CPPUTF_COMBINE_NAME_PARTS(a, b) _CPPUTF_COMBINE_NAME_PARTS2(a, b)
#define _CPPUTF_NEXT_REGISTRAR_NAME    _CPPUTF_COMBINE_NAME_PARTS(s_test_registrar_, __COUNTER__)
#define _CPPUTF_NEXT_SECTION_LOCK_NAME _CPPUTF_COMBINE_NAME_PARTS(section_lock_, __COUNTER__)
#define _CPPUTF_NEXT_EXCEPTION_NAME    _CPPUTF_COMBINE_NAME_PARTS(exception_, __COUNTER__)
#define _CPPUTF_NEXT_MAYBEUNUSED_NAME  _CPPUTF_COMBINE_NAME_PARTS(unused_, __COUNTER__)

//------------------------------------------------------------------------------------------------------------

#define TEST_CASE(TestFixture, TestName, ...) namespace {                                           \
    struct TestCase_##TestName : TestFixture, CppUnitTestFramework::CommonFixture {                 \
        using CppUnitTestFramework::CommonFixture::CommonFixture;                                   \
        static constexpr std::string_view Name = #TestFixture "::" #TestName;                       \
        static constexpr auto Tags = make_tags_array(__VA_ARGS__);                                  \
        void Run();                                                                                 \
    };                                                                                              \
    CppUnitTestFramework::TestRegistry::AutoReg<TestCase_##TestName> _CPPUTF_NEXT_REGISTRAR_NAME;   \
}                                                                                                   \
void TestCase_##TestName::Run()

//------------------------------------------------------------------------------------------------------------

#define SECTION(Text)  if (auto _CPPUTF_NEXT_SECTION_LOCK_NAME = EnterSection("Section: " Text); true)
#define SCENARIO(Text) if (auto _CPPUTF_NEXT_SECTION_LOCK_NAME = EnterSection("Scenario: " Text); true)
#define GIVEN(Text)    if (auto _CPPUTF_NEXT_SECTION_LOCK_NAME = EnterSection("Given: " Text); true)
#define AND(Text)      if (auto _CPPUTF_NEXT_SECTION_LOCK_NAME = EnterSection("And: " Text); true)
#define WHEN(Text)     if (auto _CPPUTF_NEXT_SECTION_LOCK_NAME = EnterSection("When: " Text); true)
#define THEN(Text)     if (auto _CPPUTF_NEXT_SECTION_LOCK_NAME = EnterSection("Then: " Text); true)

//------------------------------------------------------------------------------------------------------------

#define _CPPUTF_ASSERT_LOCATION CppUnitTestFramework::AssertLocation{ __FILE__, __LINE__ }

#define REQUIRE(Expression)        CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Throw, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsTrue((Expression), #Expression))
#define REQUIRE_TRUE(Expression)   CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Throw, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsTrue((Expression), #Expression))
#define REQUIRE_FALSE(Expression)  CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Throw, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsFalse((Expression), #Expression))
#define REQUIRE_EQUAL(Left, Right) CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Throw, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::AreEqual((Left), (Right)))
#define REQUIRE_NULL(Expression)   CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Throw, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsNull((Expression), #Expression))
#define REQUIRE_THROW(ExceptionType, Expression) \
    CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Throw, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::Throws<ExceptionType>([&] { Expression; }))

#define CHECK(Expression)        CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Continue, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsTrue((Expression), #Expression))
#define CHECK_TRUE(Expression)   CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Continue, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsTrue((Expression), #Expression))
#define CHECK_FALSE(Expression)  CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Continue, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsFalse((Expression), #Expression))
#define CHECK_EQUAL(Left, Right) CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Continue, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::AreEqual((Left), (Right)))
#define CHECK_NULL(Expression)   CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Continue, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::IsNull((Expression), #Expression))
#define CHECK_THROW(ExceptionType, Expression) \
    CppUnitTestFramework::CommonFixture::HandleAssert(CppUnitTestFramework::AssertType::Continue, _CPPUTF_ASSERT_LOCATION, CppUnitTestFramework::Assert::Throws<ExceptionType>([&] { Expression; }))

//------------------------------------------------------------------------------------------------------------

#define UNUSED_RETURN(Expression) [[maybe_unused]] auto _CPPUTF_NEXT_MAYBEUNUSED_NAME = Expression

//------------------------------------------------------------------------------------------------------------

#ifdef GENERATE_UNIT_TEST_MAIN
int main(int argc, const char* argv[]) {
    CppUnitTestFramework::RunOptions options;
    if (!options.ParseCommandLine(argc, argv)) {
        return -1;
    }

    CppUnitTestFramework::TestRegistry::Run(
        &options,
        CppUnitTestFramework::ConsoleLogger::Create(&options)
    );

    return 0;
}
#endif