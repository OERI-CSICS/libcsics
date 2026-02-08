
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(TEST_CSICS ALIAS CSICS)

add_library(test_utils OBJECT test_utils.cpp io/compression_utils.cpp)

set(TESTS)
set(LIBS)

if (CSICS_BUILD_QUEUE)
    list(APPEND TESTS queue/spsc_queue_test.cpp)
endif()

if (CSICS_BUILD_IO)
    if (CSICS_USE_ZSTD)
        list(APPEND TESTS io/zstd_compression_test.cpp)
    endif()
    if (CSICS_USE_ZLIB)
        list(APPEND TESTS io/zlib_compression_test.cpp)
    endif()
    find_package(OpenSSL)
    if (OPENSSL_FOUND)
        list(APPEND TESTS io/base64_encoding_test.cpp)
        list(APPEND LIBS OpenSSL::Crypto)
    else()
        find_library(CRYPTO_LIB crypto)
        if (CRYPTO_LIB)
            list(APPEND TESTS io/base64_encoding_test.cpp)
            list(APPEND LIBS ${CRYPTO_LIB})
        endif()
    endif()
endif()

add_executable(tests ${TESTS})
target_link_libraries(tests PRIVATE GTest::gtest_main GTest::gmock_main CSICS test_utils ${LIBS})

target_compile_options(tests PRIVATE ${CSICS_COMPILE_FLAGS})
add_test(NAME csics_tests COMMAND tests)

include(GoogleTest)
message(STATUS "Available tests: ${TESTS}")
gtest_discover_tests(tests)

