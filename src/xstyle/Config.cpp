/**************************************************************\
Edition:
##  @date 15/11/2026 by @author Tsukini

File Name:
##  @file Config.cpp

File Description:
##  You know, I don t think there are good or bad descriptions,
##  for me, life is all about functions...
\**************************************************************/

#define _Exception
#define _Attribute
#include <utils/utils.hpp>
#include "xstyle/Config.hpp"
#include "xstyle/Tools.hpp"
#include <functional>
#include <algorithm>
#include <optional>
#include <fstream>
#include <cctype>

/* tools */
_cold static utils::exception::ErrorException error_(const std::string& source, const std::string& message)
{
    return utils::exception::ErrorException(utils::exception::InternalCode::InvalidArgument, source + ": " + message);
}

_cold static void scan_(const std::string& line, std::string& kept, int& depth)
{
    // The line without its # comment (outside the strings), and the [ ] depth after it
    char quote = '\0';
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (quote != '\0') {
            if (quote == '"' && c == '\\' && i + 1 < line.size()) {
                kept += line.substr(i++, 2);
                continue;
            }
            if (c == quote) quote = '\0';
        } else if (c == '"' || c == '\'') {
            quote = c;
        } else if (c == '#') {
            break;
        } else if (c == '[') {
            ++depth;
        } else if (c == ']') {
            --depth;
        }
        kept += c;
    }
}

_cold static std::string scalar_(const std::string& text, std::size_t& i, const std::string& source)
{
    // "basic string" (\" \\ \n \t escapes), 'literal string', true / false or an integer
    while (i < text.size() && xstyle::is_space(text[i])) ++i;
    if (i >= text.size()) throw error_(source, "value expected");
    const char quote = text[i];
    std::string value;
    if (quote == '"' || quote == '\'') {
        for (++i; i < text.size() && text[i] != quote; ++i) {
            if (quote == '"' && text[i] == '\\' && i + 1 < text.size()) {
                const char next = text[++i];
                value += next == 'n' ? '\n' : next == 't' ? '\t' : next;
                continue;
            }
            value += text[i];
        }
        if (i >= text.size()) throw error_(source, "string not closed");
        ++i;
        return value;
    }
    while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_' || text[i] == '-' || text[i] == '+')) value += text[i++];
    if (value != "true" && value != "false" && (value.empty() || !std::all_of(value.begin() + (value[0] == '-' || value[0] == '+'), value.end(), ::isdigit)))
        throw error_(source, "'" + (value.empty() ? text.substr(i, 1) : value) + "' is not a value (\"string\", 'string', true, false, integer or [array])");
    return value;
}

_cold static std::vector<std::string> value_(const std::string& text, const std::string& source)
{
    // A scalar, or an array of scalars ([a, b,] on one or several lines)
    std::size_t i = 0;
    std::vector<std::string> values;
    while (i < text.size() && xstyle::is_space(text[i])) ++i;
    if (i < text.size() && text[i] == '[') {
        ++i;
        for (;;) {
            while (i < text.size() && (xstyle::is_space(text[i]) || text[i] == ',')) ++i;
            if (i >= text.size()) throw error_(source, "array not closed");
            if (text[i] == ']') {
                ++i;
                break;
            }
            if (text[i] == '[') throw error_(source, "nested arrays are not supported");
            values.push_back(scalar_(text, i, source));
            while (i < text.size() && xstyle::is_space(text[i])) ++i;
            if (i < text.size() && text[i] != ',' && text[i] != ']') throw error_(source, "',' or ']' expected in the array");
        }
    } else {
        values.push_back(scalar_(text, i, source));
    }
    while (i < text.size() && xstyle::is_space(text[i])) ++i;
    if (i < text.size()) throw error_(source, "unexpected '" + text.substr(i) + "' after the value");
    return values;
}

/* config */
_cold std::optional<std::filesystem::path> xstyle::find_config(const std::filesystem::path& start, const std::filesystem::path& root)
{
    std::error_code error;
    for (std::filesystem::path directory = std::filesystem::weakly_canonical(start, error); !directory.empty(); directory = directory.parent_path()) {
        if (std::filesystem::is_regular_file(directory / XCONFIG_FILE, error)) return directory / XCONFIG_FILE;
        if (directory == root || directory == directory.root_path()) break;
    }
    return std::nullopt;
}

_cold xstyle::Config xstyle::read_config(const std::filesystem::path& file)
{
    std::ifstream stream(file);
    if (!stream.is_open()) throw utils::exception::ErrorException(utils::exception::InternalCode::Read, "Can't read " + file.string());
    xstyle::Config config;
    config.file = file;

    // [[path]] table being read: match, ignore, exclude
    struct Table {
        std::string source;
        std::vector<std::string> match;
        std::vector<std::string> ignore;
        bool exclude = false;
    };
    std::optional<Table> table;
    const std::function<void(void)> close = [&]() {
        if (!table) return;
        if (table->match.empty()) throw error_(table->source, "[[path]] without match = \"<pattern>\"");
        if (table->ignore.empty() == !table->exclude) throw error_(table->source, "[[path]] needs ignore = [\"CODE\", ...] or exclude = true (not both)");
        std::string codes;
        for (const std::string& code: table->ignore)
            codes += (codes.empty() ? ":" : ",") + code;
        for (const std::string& pattern: table->match) {
            const std::optional<xstyle::PathRule> rule = xstyle::Ignore::parse(pattern + codes, file.parent_path(), table->source);
            if (rule) config.paths.push_back(*rule);
        }
        table.reset();
    };

    // One statement per line, an array can go on over the next lines
    std::string statement;
    std::string source;
    int depth = 0;
    std::size_t number = 0;
    for (std::string line; std::getline(stream, line);) {
        ++number;
        if (depth <= 0) {
            statement.clear();
            source = file.string() + ":" + std::to_string(number);
            depth = 0;
        }
        scan_(line, statement, depth);
        statement += ' ';
        if (depth > 0) continue;
        const std::string text = xstyle::trim(statement);
        if (text.empty()) continue;
        if (text.starts_with("[")) {
            close();
            if (text != "[[path]]") throw error_(source, "unknown table '" + text + "' (only [[path]])");
            table = Table{source, {}, {}, false};
            continue;
        }
        const std::size_t equal = text.find('=');
        if (equal == std::string::npos) throw error_(source, "'key = value' expected");
        std::string key = xstyle::trim(text.substr(0, equal));
        if (key.size() >= 2 && (key[0] == '"' || key[0] == '\'') && key.back() == key[0]) key = key.substr(1, key.size() - 2);
        const std::vector<std::string> values = value_(text.substr(equal + 1), source);
        if (!table) {
            config.options.push_back({key, values, source});
            continue;
        }
        if (key == "match") table->match.insert(table->match.end(), values.begin(), values.end());
        else if (key == "ignore") table->ignore.insert(table->ignore.end(), values.begin(), values.end());
        else if (key == "exclude" && values.size() == 1 && (values[0] == "true" || values[0] == "false")) table->exclude = values[0] == "true";
        else throw error_(source, "unknown key '" + key + "' in [[path]] (match, ignore, exclude = true)");
    }
    if (depth > 0) throw error_(source, "array not closed");
    close();
    return config;
}
