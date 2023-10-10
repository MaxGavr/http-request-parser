#include "http_request_parser/http_request_parser.hpp"

#include <stdexcept>

HttpRequest HttpRequest::Parse(std::string_view request_string)
{
    throw std::logic_error("not implemented");
}

HttpRequest::Method HttpRequest::GetMethod() const
{
    return m_method;
}

std::string_view HttpRequest::GetUrl() const
{
    return m_url;
}

std::optional<std::string_view> HttpRequest::GetHeader(std::string_view name) const
{
    auto it = m_headers.find(name);
    if (it == m_headers.end())
        return {};

    return it->second;
}

void HttpRequest::EnumerateHeaders(const HeaderEnumerator& enumerator) const
{
    for (auto [name, value] : m_headers)
    {
        if (!enumerator(name, value))
            return;
    }
}

HttpRequest::HttpRequest(Method method, std::string_view url, std::unordered_map<std::string_view, std::string_view> headers)
    : m_method(method)
    , m_url(url)
    , m_headers(std::move(headers))
{
}
