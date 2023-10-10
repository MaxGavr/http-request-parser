#include <catch2/catch_test_macros.hpp>

#include "http_request_parser/http_request_parser.hpp"

TEST_CASE("Parsing HTTP requests not implemented")
{
    CHECK_THROWS_AS(HttpRequest::Parse(""), std::logic_error);
}
