#include <catch2/catch_test_macros.hpp>

#include "http_request_parser/http_request_parser.hpp"

#include <set>
#include <algorithm>

TEST_CASE("Parsing HTTP request")
{
    const std::string request_string = 
        "GET /wiki/http HTTP/1.1\r\n"
        "Host: ru.wikipedia.org\r\n"
        "User-Agent: Mozilla/5.0 (X11; U; Linux i686; ru; rv:1.9b5) Gecko/2008050509 Firefox/3.0b5\r\n"
        "Accept: text/html\r\n"
        "Connection: close\r\n\r\n";

    const auto [request, error] = HttpRequest::Parse(request_string);

    REQUIRE(error == HttpRequest::ParsingResult::Success);

    CHECK(request->GetMethod() == HttpRequest::Method::Get);
    CHECK(request->GetUrl() == "/wiki/http");
    CHECK(request->GetHeader("host") == "ru.wikipedia.org");

    const std::vector<std::pair<std::string, std::string>> expected_headers = {
        {"host", "ru.wikipedia.org"},
        {"user-agent", "Mozilla/5.0 (X11; U; Linux i686; ru; rv:1.9b5) Gecko/2008050509 Firefox/3.0b5"},
        {"accept", "text/html"},
        {"connection", "close"},
    };

    std::vector<std::pair<std::string, std::string>> enumerated_headers;
    const auto enumerator = [&enumerated_headers] (std::string_view name, std::string_view value) {
        enumerated_headers.emplace_back(std::string(name), std::string(value));
        return true;
    };

    request->EnumerateHeaders(enumerator);

    const auto to_lower_string = [] (std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), [] (unsigned char c) { return std::tolower(c); });
    };

    const auto are_headers_equal = [to_lower_string] (std::pair<std::string, std::string> header_1,
                                                      std::pair<std::string, std::string> header_2)
    {
        to_lower_string(header_1.first);
        to_lower_string(header_2.first);

        return header_1.first == header_2.first && header_1.second == header_2.second;
    };

    CHECK(std::is_permutation(expected_headers.begin(), expected_headers.end(),
                              enumerated_headers.begin(), enumerated_headers.end(),
                              are_headers_equal));
}
