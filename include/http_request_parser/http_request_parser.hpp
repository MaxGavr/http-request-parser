#pragma once

#include <string_view>
#include <functional>
#include <optional>

class HttpRequest
{
public:
    static HttpRequest Parse(std::string_view request_string);

    enum class Method
    {
        Get,
        Head,
        Post,
        Put,
        Delete,
        Connect,
        Options,
        Trace,
        Patch,
    };

    Method GetMethod() const;
    std::string_view GetUrl() const;
    std::optional<std::string_view> GetHeader(std::string_view name) const;

    using HeaderEnumerator = std::function<bool(std::string_view header_name, std::string_view header_value)>;
    void EnumerateHeaders(const HeaderEnumerator& enumerator) const;
};
