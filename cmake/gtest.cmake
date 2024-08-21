enable_testing()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
  DOWNLOAD_EXTRACT_TIMESTAMP True
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add test files
set(TEST_SOURCES
    test/cpp/CanDevice_test.cpp
    test/cpp/CanFrame_test.cpp
    test/cpp/CanExtra_test.cpp
    test/cpp/CanStats_test.cpp
    test/cpp/LogIt_test.cpp
)


add_executable(CanModuleTest ${TEST_SOURCES})

target_link_libraries(
  CanModuleTest gtest_main CanModuleMain LogIt
)

target_include_directories(CanModuleTest PRIVATE ${logit_SOURCE_DIR}/include)

include(GoogleTest)
gtest_discover_tests(CanModuleTest)
