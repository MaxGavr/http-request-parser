#include "http_request_parser/http_request_parser.hpp"

namespace
{
bool IsSpace(char c)
{
    return c == ' ';
}

bool IsCR(char c)
{
    return c == '\r';
}

bool IsLF(char c)
{
    return c == '\n';
}

bool IsVisual(char c)
{
    return std::isprint(c) && !IsSpace(c);
}

bool IsDelimiter(char c)
{
    switch (c)
    {
    case '(': case ')': case '<': case '>': case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?': case '=':
    case '{': case '}': 
        return true;
    
    default:
        return false;
    }
}

bool IsTokenChar(char c)
{
    return IsVisual(c) && !IsDelimiter(c);
}

[[noreturn]] void ThrowError(const char* message)
{
    throw ParsingError(message);
}

HttpRequest::Method ParseMethod(std::string_view method)
{
    static std::unordered_map<std::string_view, HttpRequest::Method> known_methods = {
        {"GET", HttpRequest::Method::Get},
        {"POST", HttpRequest::Method::Post},
        {"PUT", HttpRequest::Method::Put},
        {"CONNECT", HttpRequest::Method::Connect},
        {"HEAD", HttpRequest::Method::Head},
    };

    auto it = known_methods.find(method);
    if (it == known_methods.end())
        ThrowError("unknown method");

    return it->second;
}

}

HttpRequest HttpRequest::Parse(std::string request_string)
{
    std::optional<Method> method;
    std::optional<std::string_view> url;
    std::unordered_map<std::string_view, std::string_view> headers;

    enum class State
    {
        MethodStart,
        Method,

        UrlStart,
        Url,

        VersionStart,
        Version,

        StartLineEnd,

        HeadersStart,

        HeaderNameStart,
        HeaderName,
        HeaderDelimiter,
        HeaderValueStart,
        HeaderValue,
        HeaderLineEnd,

        HeadersLineEnding,

        Finished,
    };

    State state = State::MethodStart;

    auto iterator = request_string.begin();
    const auto end = request_string.end();

    std::string::const_iterator token_start;
    std::string::const_iterator token_end;

    std::string_view header_name;
    std::string_view header_value;

    const auto get_token = [&token_start, &token_end] {
        return std::string_view(&(*token_start), std::distance(token_start, token_end));
    };

    while (iterator != end)
    {
        const char c = *iterator;

        switch (state)
        {
            case State::MethodStart:
            {
                if (!IsTokenChar(c))
                    ThrowError("invalid method");
                
                token_start = iterator;

                state = State::Method;
                break;
            }
            case State::Method:
            {
                if (IsTokenChar(c))
                    break;

                if (IsSpace(c))
                {
                    token_end = iterator;
                    method = ParseMethod(get_token());

                    state = State::UrlStart;
                    break;
                }

                ThrowError("invalid method");
            }
            case State::UrlStart:
            {
                if (!IsVisual(c) && !IsDelimiter(c))
                    ThrowError("invalid URL");

                token_start = iterator;
                state = State::Url;

                break;
            }
            case State::Url:
            {
                if (IsVisual(c) || IsDelimiter(c))
                    break;

                if (IsSpace(c))
                {
                    token_end = iterator;
                    url = get_token();

                    state = State::VersionStart;
                    break;
                }

                ThrowError("invalid url");
            }
            case State::VersionStart:
            {
                if (!IsVisual(c) && !IsDelimiter(c))
                    ThrowError("invalid version");

                token_start = iterator;
                state = State::Version;
                break;
            }
            case State::Version:
            {
                if (IsVisual(c) || IsDelimiter(c))
                    break;

                if (IsCR(c))
                {
                    token_end = iterator;
                    state = State::StartLineEnd;
                    break;
                }

                ThrowError("invalid version");
            }
            case State::StartLineEnd:
            {
                if (!IsLF(c))
                    ThrowError("invalid start line ending");

                state = State::HeaderNameStart;
                break;
            }
            case State::HeaderNameStart:
            {
                if (IsCR(c))
                {
                    state = State::HeadersLineEnding;
                    break;
                }

                if (!IsTokenChar(c))
                    ThrowError("invalid header name");

                token_start = iterator;
                *iterator = std::tolower(*iterator);

                state = State::HeaderName;

                break;
            }
            case State::HeaderName:
            {
                if (IsTokenChar(c))
                {
                    *iterator = std::tolower(*iterator);
                    break;
                }

                if (c == ':')
                {
                    token_end = iterator;
                    header_name = get_token();

                    state = State::HeaderDelimiter;

                    break;
                }

                ThrowError("invalid header name");
            }
            case State::HeaderDelimiter:
            {
                if (!IsSpace(c))
                    ThrowError("invalid header delimiter");

                state = State::HeaderValueStart;

                break;
            }
            case State::HeaderValueStart:
            {
                if (!IsVisual(c))
                    ThrowError("invalid header value");

                token_start = iterator;
                state = State::HeaderValue;

                break;
            }
            case State::HeaderValue:
            {
                if (IsVisual(c) || IsSpace(c))
                    break;

                if (IsCR(c))
                {
                    token_end = iterator;

                    header_value = get_token();
                    headers.emplace(header_name, header_value);

                    state = State::HeaderLineEnd;
                    break;
                }

                ThrowError("invalid header value");
            }
            case State::HeaderLineEnd:
            {
                if (!IsLF(c))
                    ThrowError("invalid header line ending");

                state = State::HeaderNameStart;
                break;
            }
            case State::HeadersLineEnding:
            {
                if (!IsLF(c))
                    ThrowError("invalid headers line ending");

                state = State::Finished;
                break;
            }
        }

        ++iterator;
    }

    if (state != State::Finished)
        throw ParsingError("parsing failed");

    return {std::move(request_string), *method, *url, std::move(headers)};
}

HttpRequest::Method HttpRequest::GetMethod() const
{
    return m_method;
}

std::string_view HttpRequest::GetUrl() const
{
    return m_url;
}

std::optional<std::string_view> HttpRequest::GetHeader(std::string name) const
{
    for (auto& c : name)
        c = std::tolower(c);

    const auto it = m_headers.find(name);
    if (it == m_headers.end())
        return {};

    return it->second;
}

void HttpRequest::EnumerateHeaders(const HeaderEnumerator& enumerator) const
{
    for (const auto& [name, value] : m_headers)
    {
        if (!enumerator(name, value))
            return;
    }
}

HttpRequest::HttpRequest(std::string&& data, Method method, std::string_view url, std::unordered_map<std::string_view, std::string_view>&& headers)
    : m_data(std::move(data))
    , m_method(method)
    , m_url(url)
    , m_headers(std::move(headers))
{
}
