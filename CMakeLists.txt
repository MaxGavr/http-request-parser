cmake_minimum_required(VERSION 3.14)

project(http_request_parser LANGUAGES CXX)

add_library(http_request_parser
    source/http_request_parser.cpp
)

target_include_directories(http_request_parser PUBLIC include)
target_compile_features(http_request_parser PUBLIC cxx_std_17)

#
# Tests
#

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(http_request_parser_test
    source/http_request_parser_test.cpp
)
target_link_libraries(http_request_parser_test
    PRIVATE http_request_parser
    PRIVATE Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(http_request_parser_test)
