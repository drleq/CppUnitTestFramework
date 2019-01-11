# Common compiler flags
if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    add_compile_options(
        -Wall          # Enable all warnings
        -Wextra
        -pedantic
        -Wcast-align
        -Wcast-qual
        -Wctor-dtor-privacy
        -Wdisabled-optimization
        -Wformat=2
        -Winit-self
        -Wlogical-op
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wnoexcept
        -Wold-style-cast
        -Woverloaded-virtual
        -Wredundant-decls
        -Wshadow
        -Wstrict-null-sentinel
        -Wstrict-overflow
        -Wundef
        -Wno-missing-field-initializers # Allow implict zero initialization of structs
        -Werror        # Treat warnings as errors
    )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_compile_options(
        -W4            # Enable all meaningful warnings
        -WX            # Treat warnings as errors
    )
else()
    message(FATAL_ERROR "Unsupported compiler.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source files to executable
add_executable(Tests
    main.cpp
    SectionTest.cpp
    TestCaseTest.cpp
    ToStringTest.cpp
    ../CppUnitTestFramework.hpp)

# Configure the include directories
target_include_directories(Tests
    PUBLIC .
    PUBLIC ..)