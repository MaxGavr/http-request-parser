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
}

std::pair<std::optional<HttpRequest>, HttpRequest::ParsingResult> HttpRequest::Parse(std::string request_string)
{
    const auto error = std::make_pair(std::optional<HttpRequest>(), HttpRequest::ParsingResult::Error);

    std::optional<Method> method;
    std::optional<std::string_view> url;
    std::unordered_map<std::string_view, std::string_view> headers;

    enum class State
    {
        MethodStart,

        MethodGet,
        MethodP,
        MethodPost,
        MethodPut,
        MethodConnect,
        MethodHead,

        UrlStart,
        Url,

        VersionStart,
        Version,

        RequestLineEnding,

        HeaderNameStart,
        HeaderName,
        HeaderDelimiter,
        HeaderValueStart,
        HeaderValue,
        HeaderLineEnding,

        HeadersLineEnding,

        Finished,
    };

    State state = State::MethodStart;

    auto iterator = request_string.begin();
    const auto end = request_string.end();

    size_t method_index = 1;

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
                switch (std::tolower(c))
                {
                    case 'c':
                        method = Method::Connect;
                        state = State::MethodConnect;
                        break;
                    case 'g':
                        method = Method::Get;
                        state = State::MethodGet;
                        break;
                    case 'h':
                        method = Method::Head;
                        state = State::MethodHead;
                        break;
                    case 'p':
                        state = State::MethodP;
                        break;
                    default:
                        return error;
                }

                break;
            }
            case State::MethodConnect:
            {
                constexpr std::string_view method_str = "connect";

                if (method_index < method_str.size())
                {
                    if (std::tolower(c) != method_str[method_index])
                        return error;

                    ++method_index;
                    break;
                }

                if (IsSpace(c))
                {
                    state = State::UrlStart;
                    break;
                }

                return error;
            }
            case State::MethodGet:
            {
                constexpr std::string_view method_str = "get";

                if (method_index < method_str.size())
                {
                    if (std::tolower(c) != method_str[method_index])
                        return error;

                    ++method_index;
                    break;
                }

                if (IsSpace(c))
                {
                    state = State::UrlStart;
                    break;
                }

                return error;
            }
            case State::MethodHead:
            {
                constexpr std::string_view method_str = "head";

                if (method_index < method_str.size())
                {
                    if (std::tolower(c) != method_str[method_index])
                        return error;

                    ++method_index;
                    break;
                }

                if (IsSpace(c))
                {
                    state = State::UrlStart;
                    break;
                }

                return error;
            }
            case State::MethodP:
            {
                switch (std::tolower(c))
                {
                    case 'o':
                        state = State::MethodPost;
                        method = Method::Post;
                        ++method_index;
                        break;
                    case 'u':
                        state = State::MethodPut;
                        method = Method::Put;
                        ++method_index;
                        break;
                    default:
                        return error;
                }

                break;
            }
            case State::MethodPost:
            {
                constexpr std::string_view method_str = "post";

                if (method_index < method_str.size())
                {
                    if (std::tolower(c) != method_str[method_index])
                        return error;

                    ++method_index;
                    break;
                }

                if (IsSpace(c))
                {
                    state = State::UrlStart;
                    break;
                }

                return error;
            }
            case State::MethodPut:
            {
                constexpr std::string_view method_str = "put";

                if (method_index < method_str.size())
                {
                    if (std::tolower(c) != method_str[method_index])
                        return error;

                    ++method_index;
                    break;
                }

                if (IsSpace(c))
                {
                    state = State::UrlStart;
                    break;
                }

                return error;
            }
            case State::UrlStart:
            {
                if (!IsVisual(c) && !IsDelimiter(c))
                    return error;

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

                return error;
            }
            case State::VersionStart:
            {
                if (!IsVisual(c) && !IsDelimiter(c))
                    return error;

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
                    state = State::RequestLineEnding;

                    break;
                }

                return error;
            }
            case State::RequestLineEnding:
            {
                if (!IsLF(c))
                    return error;

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
                    return error;

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

                return error;
            }
            case State::HeaderDelimiter:
            {
                if (!IsSpace(c))
                    return error;

                state = State::HeaderValueStart;

                break;
            }
            case State::HeaderValueStart:
            {
                if (!IsVisual(c))
                    return error;

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

                    state = State::HeaderLineEnding;

                    break;
                }

                return error;
            }
            case State::HeaderLineEnding:
            {
                if (!IsLF(c))
                    return error;

                state = State::HeaderNameStart;

                break;
            }
            case State::HeadersLineEnding:
            {
                if (!IsLF(c))
                    return error;

                state = State::Finished;

                break;
            }
            case State::Finished:
            {
                break;
            }
        }

        ++iterator;
    }

    if (state != State::Finished)
        return error;

    return {
        HttpRequest(std::move(request_string), *method, *url, std::move(headers)),
        ParsingResult::Success
    };
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
