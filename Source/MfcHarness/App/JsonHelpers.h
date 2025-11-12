#pragma once

#include <string>
#include <optional>

std::optional<std::string> JsonExtractString(const std::string& body, const std::string& key);
std::optional<int> JsonExtractInt(const std::string& body, const std::string& key);
std::optional<bool> JsonExtractBool(const std::string& body, const std::string& key);
