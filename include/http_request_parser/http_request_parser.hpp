#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <optional>
#include <unordered_map>

class HttpRequest
{
public:
    enum class ParsingResult
    {
        Success,
        Error,
    };

    static std::pair<std::optional<HttpRequest>, HttpRequest::ParsingResult> Parse(std::string request_string);

    enum class Method
    {
        Connect,
        Get,
        Head,
        Post,
        Put,
    };

    Method GetMethod() const;
    std::string_view GetUrl() const;
    std::optional<std::string_view> GetHeader(std::string name) const;

    using HeaderEnumerator = std::function<bool(std::string_view header_name, std::string_view header_value)>;
    void EnumerateHeaders(const HeaderEnumerator& enumerator) const;

private:
    HttpRequest(std::string&& data, Method method, std::string_view url, std::unordered_map<std::string_view, std::string_view>&& headers);

    std::string m_data;

    Method m_method;
    std::string_view m_url;
    std::unordered_map<std::string_view, std::string_view> m_headers;
};
