#include "pch.h"
#include "JsonHelpers.h"

namespace
{
std::string StripQuotes(const std::string& value)
{
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        return value.substr(1, value.size() - 2);
    }
    return value;
}
}

std::optional<std::string> JsonExtractString(const std::string& body, const std::string& key)
{
    const std::string token = "\"" + key + "\"";
    const auto pos = body.find(token);
    if (pos == std::string::npos)
    {
        return std::nullopt;
    }
    auto colon = body.find(':', pos + token.size());
    if (colon == std::string::npos)
    {
        return std::nullopt;
    }
    auto start = body.find_first_not_of(" \t\r\n", colon + 1);
    if (start == std::string::npos)
    {
        return std::nullopt;
    }
    bool quoted = body[start] == '"';
    auto end = quoted ? body.find('"', start + 1) : body.find_first_of(",}\r\n", start);
    if (end == std::string::npos)
    {
        end = body.size();
    }
    std::string value = body.substr(start, end - start + (quoted ? 1 : 0));
    value = StripQuotes(value);
    return value;
}

std::optional<int> JsonExtractInt(const std::string& body, const std::string& key)
{
    auto value = JsonExtractString(body, key);
    if (!value)
    {
        return std::nullopt;
    }
    try
    {
        return std::stoi(*value);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<bool> JsonExtractBool(const std::string& body, const std::string& key)
{
    auto value = JsonExtractString(body, key);
    if (!value)
    {
        return std::nullopt;
    }
    if (_stricmp(value->c_str(), "true") == 0)
    {
        return true;
    }
    if (_stricmp(value->c_str(), "false") == 0)
    {
        return false;
    }
    return std::nullopt;
}
