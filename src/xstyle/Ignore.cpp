/**************************************************************\
Edition:
##  @date 15/11/2026 by @author Tsukini

File Name:
##  @file Ignore.cpp

File Description:
##  You know, I don t think there are good or bad descriptions,
##  for me, life is all about functions...
\**************************************************************/

#define _Exception
#define _Attribute
#include <utils/utils.hpp>
#include "xstyle/Ignore.hpp"
#include "xstyle/Rules.hpp"
#include "xstyle/Tools.hpp"
#include "xstyle/Git.hpp"
#include <functional>
#include <algorithm>
#include <fstream>
#include <regex>

/* tools */
_cold static std::filesystem::path normal_(const std::filesystem::path& path)
{
    std::error_code error;
    const std::filesystem::path canonical = std::filesystem::weakly_canonical(std::filesystem::absolute(path, error), error);
    return error ? std::filesystem::absolute(path).lexically_normal() : canonical;
}

_hot static bool match_segments_(const std::vector<std::string>& pattern, const std::size_t i, const std::vector<std::string>& path, const std::size_t j)
{
    // ** matches any number of directories (none included), the other segments one name each (* and ? jokers)
    if (i == pattern.size()) return j == path.size();
    if (pattern[i] == "**") {
        for (std::size_t k = j; k <= path.size(); ++k)
            if (match_segments_(pattern, i + 1, path, k)) return true;
        return false;
    }
    if (j == path.size()) return false;
    return xstyle::glob_match(pattern[i], path[j]) && match_segments_(pattern, i + 1, path, j + 1);
}

/* parse */
_cold std::optional<xstyle::PathRule> xstyle::Ignore::parse(const std::string& line, const std::filesystem::path& base, const std::string& source)
{
    // [!]pattern[/][:CODE,...] like a .gitignore line; # comment, empty line ignored
    static const std::regex code(R"(^(\*|[A-Z][A-Z0-9]*(-[A-Z0-9]+)*-?\*?)$)");
    std::string text = xstyle::trim(line);
    if (text.empty() || text[0] == '#') return std::nullopt;
    xstyle::PathRule rule;
    rule.base = normal_(base);
    rule.source = source;

    // :CODE,... at the end: these rules are ignored there (the paths stay checked)
    const std::size_t colon = text.rfind(':');
    if (colon != std::string::npos) {
        const std::string codes = xstyle::trim(text.substr(colon + 1));
        for (const std::string& part: xstyle::split(codes, ',')) {
            if (!std::regex_match(part, code))
                throw utils::exception::ErrorException(utils::exception::InternalCode::InvalidArgument, source + ": '" + part + "' is not a rule code (CPP-NULL), a prefix (CPP) or a pattern (LU-*)");
            rule.codes.push_back(part);
        }
        if (rule.codes.empty()) throw utils::exception::ErrorException(utils::exception::InternalCode::InvalidArgument, source + ": no rule code after the ':'");
        text = xstyle::trim(text.substr(0, colon));
    }
    if (text.starts_with("!")) {
        rule.negate = true;
        text = text.substr(1);
    }
    if (text.starts_with("\\")) text = text.substr(1); // \# \! : a name starting with # or !
    while (text.size() > 1 && text.ends_with("/")) {
        rule.directoryOnly = true;
        text.pop_back();
    }
    const bool anchored = text.find('/') != std::string::npos; // a / (not the last one): relative to the base, else a name at any depth
    for (const std::string& segment: xstyle::split(text, '/'))
        if (segment != ".") rule.segments.push_back(segment);
    if (rule.segments.empty()) return std::nullopt;
    if (!anchored) rule.segments.insert(rule.segments.begin(), "**");
    return rule;
}

/* rules */
_cold const std::vector<xstyle::PathRule>& xstyle::Ignore::directory_(const std::filesystem::path& directory) const
{
    auto it = this->_cache.find(directory);
    if (it != this->_cache.end()) return it->second;
    std::vector<xstyle::PathRule>& rules = this->_cache[directory];
    std::ifstream file(directory / XIGNORE_FILE);
    std::size_t number = 0;
    for (std::string line; std::getline(file, line);) {
        const std::optional<xstyle::PathRule> rule = xstyle::Ignore::parse(line, directory, (directory / XIGNORE_FILE).string() + ":" + std::to_string(++number));
        if (rule) rules.push_back(*rule);
    }
    return rules;
}

_cold std::vector<const xstyle::PathRule*> xstyle::Ignore::rules_(const std::filesystem::path& path) const
{
    // [[path]] of .xconfig, then the .xignore files from the root down to the directory of the path, then --ignore-path: the last match wins
    std::vector<const xstyle::PathRule*> rules;
    for (const xstyle::PathRule& rule: this->_first)
        rules.push_back(&rule);
    const std::filesystem::path relative = path.lexically_relative(this->_root);
    if (this->_xignore && !relative.empty() && !relative.generic_string().starts_with("..")) {
        std::filesystem::path directory = this->_root;
        std::vector<std::filesystem::path> parts(relative.begin(), relative.end());
        for (std::size_t i = 0; i < parts.size(); ++i) {
            for (const xstyle::PathRule& rule: this->directory_(directory))
                rules.push_back(&rule);
            directory /= parts[i];
        }
    }
    for (const xstyle::PathRule& rule: this->_last)
        rules.push_back(&rule);
    return rules;
}

_hot bool xstyle::Ignore::matches_(const xstyle::PathRule& rule, const std::filesystem::path& path, const bool directory)
{
    if (rule.directoryOnly && !directory) return false;
    const std::filesystem::path relative = path.lexically_relative(rule.base);
    if (relative.empty() || relative.generic_string().starts_with("..")) return false;
    std::vector<std::string> parts;
    for (const std::filesystem::path& part: relative)
        if (part != ".") parts.push_back(part.string());
    return match_segments_(rule.segments, 0, parts, 0);
}

/* setup */
_cold void xstyle::Ignore::loadGit(const std::filesystem::path& repository)
{
    const std::filesystem::path root = normal_(repository);
    const xstyle::git::Result result = xstyle::git::run(root, {"ls-files", "--others", "--ignored", "--exclude-standard", "--directory", "-z"});
    if (result.code != 0) return;
    std::size_t start = 0;
    for (std::size_t end = result.out.find('\0'); end != std::string::npos; start = end + 1, end = result.out.find('\0', start)) {
        std::string entry = result.out.substr(start, end - start);
        while (entry.ends_with("/")) entry.pop_back();
        if (!entry.empty()) this->_gitIgnored.insert((root / entry).lexically_normal().generic_string());
    }
}

/* query */
_hot bool xstyle::Ignore::excluded(const std::filesystem::path& path, const bool directory) const
{
    // Excluded when it, or one of its directories under the root, matches last a rule without codes (a !rule includes again)
    const std::filesystem::path normal = normal_(path);
    const std::function<bool(const std::filesystem::path&, const bool)> evaluate = [this](const std::filesystem::path& p, const bool isDirectory) {
        bool result = false;
        for (const xstyle::PathRule* rule: this->rules_(p))
            if (rule->codes.empty() && xstyle::Ignore::matches_(*rule, p, isDirectory)) result = !rule->negate;
        return result;
    };
    const std::filesystem::path relative = normal.lexically_relative(this->_root);
    if (!relative.empty() && !relative.generic_string().starts_with("..")) {
        std::filesystem::path parent = this->_root;
        std::vector<std::filesystem::path> parts(relative.begin(), relative.end());
        for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
            parent /= parts[i];
            if (evaluate(parent, true)) return true;
        }
    }
    return evaluate(normal, directory);
}

_hot bool xstyle::Ignore::gitIgnored(const std::filesystem::path& path) const
{
    if (!this->_git || this->_gitIgnored.empty()) return false;
    for (std::filesystem::path p = normal_(path); !p.empty() && p != p.root_path(); p = p.parent_path())
        if (this->_gitIgnored.contains(p.generic_string())) return true;
    return false;
}

_hot std::vector<std::string> xstyle::Ignore::codes(const std::filesystem::path& file) const
{
    // Codes of the rules matching the file or one of its directories, in order (a !rule removes its codes)
    const std::filesystem::path normal = normal_(file);
    std::vector<std::string> codes;
    for (const xstyle::PathRule* rule: this->rules_(normal)) {
        if (rule->codes.empty()) continue;
        bool match = xstyle::Ignore::matches_(*rule, normal, false);
        for (std::filesystem::path p = normal.parent_path(); !match && p != rule->base && p.has_relative_path(); p = p.parent_path())
            match = xstyle::Ignore::matches_(*rule, p, true);
        if (!match) continue;
        for (const std::string& code: rule->codes) {
            std::erase(codes, code);
            if (!rule->negate) codes.push_back(code);
        }
    }
    return codes;
}

/* constructor */
xstyle::Ignore::Ignore(const std::filesystem::path& root, const bool xignore)
: _root(normal_(root)), _xignore(xignore)
{
    /* Nothing */
}
