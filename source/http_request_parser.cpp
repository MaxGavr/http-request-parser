#include "http_request_parser/http_request_parser.hpp"

#include <stdexcept>

HttpRequest HttpRequest::Parse(std::string_view request_string)
{
    throw std::logic_error("not implemented");
}

HttpRequest::Method HttpRequest::GetMethod() const
{
    throw std::logic_error("not implemented");
}

std::string_view HttpRequest::GetUrl() const
{
    throw std::logic_error("not implemented");
}

std::optional<std::string_view> HttpRequest::GetHeader(std::string_view name) const
{
    return {};
}

void HttpRequest::EnumerateHeaders(const HeaderEnumerator& enumerator) const
{
}
