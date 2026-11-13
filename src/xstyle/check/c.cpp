/**************************************************************\
Edition:
##  @date 13/11/2026 by @author Tsukini

File Name:
##  @file c.cpp

File Description:
##  Rules of the C only: names (snake_case, struct x_s / x_t),
##  pointer parameters checked first, length and nesting of the functions.
\**************************************************************/

#define _Attribute
#include <utils/utils.hpp>
#include "xstyle/check/Checks.hpp"
#include "xstyle/Tools.hpp"
#include <unordered_set>
#include <functional>
#include <cctype>
#include <regex>

/* tools */
_hot static bool snake_(const std::string& name)
{
    static const std::regex snake(R"(^_?[a-z][a-z0-9_]*$)");
    return std::regex_match(name, snake);
}

_hot static std::string to_snake_(const std::string& name)
{
    std::string snake;
    for (std::size_t i = 0; i < name.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        const bool lowerBefore = i > 0 && (std::islower(static_cast<unsigned char>(name[i - 1])) || std::isdigit(static_cast<unsigned char>(name[i - 1])));
        if (std::isupper(c) && lowerBefore) snake += '_';
        snake += static_cast<char>(std::tolower(c));
    }
    return snake;
}

_hot static std::size_t function_name_(const std::string& header, std::string& name) // index of the ( of the parameters (npos if none)
{
    static const std::regex call(R"((\w+)\s*\()");
    std::smatch match;
    if (!std::regex_search(header, match, call)) return std::string::npos;
    name = match[1].str();
    return static_cast<std::size_t>(match.position(0) + match.length(0) - 1);
}

_hot static std::vector<std::string> parameters_(const std::string& header, const std::size_t open) // the text of each parameter
{
    std::vector<std::string> parameters;
    std::string current;
    int depth = 0;

    for (std::size_t j = open + 1; j < header.size(); ++j) {
        const char c = header[j];
        if (c == '(' || c == '[') ++depth;
        if ((c == ')' || c == ']') && depth-- == 0) break;
        if (c == ',' && depth == 0) {
            parameters.push_back(xstyle::trim(current));
            current.clear();
            continue;
        }
        current += c;
    }
    if (!xstyle::trim(current).empty()) parameters.push_back(xstyle::trim(current));
    return parameters;
}

_hot static std::string parameter_name_(const std::string& parameter, bool& pointer) // last identifier ((*name)(...) for a function pointer)
{
    static const std::regex functionPointer(R"(\(\s*\*\s*(\w+)\s*\))");
    std::smatch match;
    pointer = parameter.find('*') != std::string::npos || parameter.find('[') != std::string::npos;
    if (std::regex_search(parameter, match, functionPointer)) return match[1].str();

    std::string text = parameter.substr(0, parameter.find('['));
    std::size_t end = text.size();
    while (end > 0 && !xstyle::is_word(text[end - 1])) --end;
    std::size_t start = end;
    while (start > 0 && xstyle::is_word(text[start - 1])) --start;
    const std::string before = xstyle::trim(text.substr(0, start));
    if (before.empty() || before == "const" || before == "struct" || before == "unsigned" || before == "signed") return ""; // a type alone (no name)
    return text.substr(start, end - start);
}

_hot static bool keyword_(const std::string& word)
{
    static const std::unordered_set<std::string> keywords = {"return", "goto", "case", "else", "sizeof", "typedef", "if", "for", "while", "switch", "do", "break",
        "continue", "default", "extern", "static", "const", "volatile", "register", "inline", "struct", "union", "enum", "unsigned", "signed"};
    return keywords.contains(word);
}

/* checks: names */
_hot static void types_(const xstyle::SourceFile& file, xstyle::check::Issues& issues)
{
    static const std::regex tag(R"(\b(struct|union|enum)\s+(\w+)\s*$)");
    static const std::regex typedefSimple(R"(^\s*typedef\b[^;{}]*?(?:\(\s*\*\s*(\w+)\s*\)\s*\(.*\)|\b(\w+)\s*(?:\[[^\]]*\]\s*)*)\s*;)");
    static const std::regex closing(R"(^\s*\}\s*(\w+)\s*;)");
    static const std::regex enumValue(R"(^\s*([A-Za-z_]\w*)\s*(?:=[^,]*)?,?\s*$)");
    static const std::regex upper(R"(^[A-Z][A-Z0-9_]*$)");
    static const std::regex typedefName(R"(^_?[a-z][a-z0-9_]*_t$)");
    const std::vector<std::string>& code = file.getCode();
    const std::vector<xstyle::Brace>& braces = file.getBraces();
    const std::vector<xstyle::LineInfo>& info = file.getInfo();

    // Typedef name: snake_case ending with _t
    const std::function<void(std::size_t, std::size_t, const std::string&)> typedef_ = [&](const std::size_t line, const std::size_t column, const std::string& name) {
        if (!std::regex_match(name, typedefName))
            issues.push_back(xstyle::check::make_issue(file, line, column, "C-TYPE-NAME", "Typedef " + name + " not in snake_case ending with _t",
                "typedef ... " + to_snake_(name.ends_with("_t") ? name.substr(0, name.size() - 2) : name) + "_t;"));
    };

    for (const xstyle::Brace& brace: braces) {
        if (brace.kind != xstyle::Scope::Struct && brace.kind != xstyle::Scope::Enum) continue;
        std::smatch match;
        if (std::regex_search(brace.header, match, tag)) {
            const std::string kind = match[1].str();
            const std::string name = match[2].str();
            const std::string suffix = std::string("_") + kind[0];
            if (!snake_(name) || !name.ends_with(suffix)) {
                const std::size_t column = code[brace.headerLine].find(name);
                issues.push_back(xstyle::check::make_issue(file, brace.headerLine, column == std::string::npos ? 0 : column, "C-TYPE-NAME",
                    kind + " " + name + " not in snake_case ending with " + suffix,
                    kind + " " + to_snake_(name.ends_with(suffix) ? name.substr(0, name.size() - 2) : name) + suffix + " {"));
            }
        }
        // typedef struct x_s {...} x_t;
        if (brace.header.starts_with("typedef") && brace.closeLine != NO_INDEX && std::regex_search(code[brace.closeLine], match, closing))
            typedef_(brace.closeLine, static_cast<std::size_t>(match.position(1)), match[1].str());

        // enum values in UPPER_SNAKE
        if (brace.kind != xstyle::Scope::Enum || brace.closeLine == NO_INDEX) continue;
        for (std::size_t i = brace.openLine + 1; i < brace.closeLine; ++i) {
            if (info[i].preprocessor || !std::regex_match(code[i], match, enumValue)) continue;
            if (!std::regex_match(match[1].str(), upper))
                issues.push_back(xstyle::check::make_issue(file, i, static_cast<std::size_t>(match.position(1)), "C-TYPE-NAME", "Enum value " + match[1].str() + " not in UPPER_SNAKE",
                    "rename it in UPPER_SNAKE"));
        }
    }

    // typedef <type> name; / typedef T (*name)(...);
    for (std::size_t i = 0; i < code.size(); ++i) {
        std::smatch match;
        if (info[i].preprocessor || code[i].find("typedef") == std::string::npos || code[i].find('{') != std::string::npos) continue;
        if (!std::regex_search(code[i], match, typedefSimple)) continue;
        const int group = match[1].matched ? 1 : 2;
        typedef_(i, static_cast<std::size_t>(match.position(group)), match[group].str());
    }
}

_hot static void names_(const xstyle::SourceFile& file, xstyle::check::Issues& issues)
{
    static const std::regex declaration(R"(^\s*(?:(?:const|static|volatile|register|unsigned|signed|struct|union|enum|long|short|extern)\s+)*(\w+)(?:\s+const)?(?:\s*\*+\s*|\s+)(?:const\s+)?(\w+)\s*(=|;|\[|,))");
    static const std::regex upper(R"(^[A-Z][A-Z0-9_]*$)");
    const std::vector<std::string>& code = file.getCode();
    const std::vector<xstyle::LineInfo>& info = file.getInfo();
    const std::vector<xstyle::Brace>& braces = file.getBraces();
    std::unordered_set<std::string> reported;

    const std::function<void(std::size_t, std::size_t, const std::string&, const std::string&)> report = [&](const std::size_t line, const std::size_t column,
        const std::string& what, const std::string& name) {
        if (name.empty() || snake_(name) || name == "main" || !reported.insert(what + name + std::to_string(line)).second) return;
        issues.push_back(xstyle::check::make_issue(file, line, column, "C-FUNC-NAME", what + " " + name + " not in snake_case", "rename it " + to_snake_(name)));
    };

    // Functions & their parameters (definitions)
    for (const xstyle::Brace& brace: braces) {
        if (brace.kind != xstyle::Scope::Function) continue;
        std::string name;
        const std::size_t open = function_name_(brace.header, name);
        if (open == std::string::npos) continue;
        const std::size_t line = brace.headerLine == NO_INDEX ? brace.openLine : brace.headerLine;
        const std::size_t column = code[line].find(name + "(") != std::string::npos ? code[line].find(name + "(") : code[line].find(name);
        report(line, column == std::string::npos ? 0 : column, "Function", name);
        for (const std::string& parameter: parameters_(brace.header, open)) {
            bool pointer = false;
            const std::string parameterName = parameter_name_(parameter, pointer);
            report(line, 0, "Parameter", parameterName);
        }
    }

    // Prototypes, globals (outside of a function) & locals (in a body): <type> name = / ; / [ / ,
    for (std::size_t i = 0; i < code.size(); ++i) {
        std::smatch match;
        if (info[i].preprocessor || info[i].parenDepth > 0 || xstyle::is_blank(code[i]) || code[i].find("typedef") != std::string::npos) continue;
        const xstyle::Scope scope = file.scopeAt(i);
        if (scope == xstyle::Scope::Struct || scope == xstyle::Scope::Enum || scope == xstyle::Scope::Init) continue;
        if (!info[i].inFunction && code[i].find('(') != std::string::npos) {
            static const std::regex prototype(R"(^\s*(?:(?:static|extern|inline|const|unsigned|signed|struct|union|enum|long|short)\s+)*\w+[\s*]+(\w+)\s*\([^;{]*\)\s*;)");
            if (std::regex_search(code[i], match, prototype)) report(i, static_cast<std::size_t>(match.position(1)), "Function", match[1].str());
            continue;
        }
        if (!std::regex_search(code[i], match, declaration) || keyword_(match[1].str()) || keyword_(match[2].str())) continue;
        const std::string name = match[2].str();
        if (!info[i].inFunction && code[i].find("const") != std::string::npos && std::regex_match(name, upper)) continue; // global constant
        report(i, static_cast<std::size_t>(match.position(2)), info[i].inFunction ? "Variable" : "Global", name);
    }
}

/* checks: functions */
_hot static void pointers_(const xstyle::SourceFile& file, xstyle::check::Issues& issues)
{
    static const std::regex condition(R"(\b(if|while|assert)\b|\?)");
    const std::vector<std::string>& code = file.getCode();
    const std::vector<xstyle::LineInfo>& info = file.getInfo();
    const std::vector<xstyle::Brace>& braces = file.getBraces();

    for (std::size_t b = 0; b < braces.size(); ++b) {
        const xstyle::Brace& brace = braces[b];
        if (brace.kind != xstyle::Scope::Function || brace.isStatic || brace.closeLine == NO_INDEX || brace.closeLine == brace.openLine) continue;
        std::string name;
        const std::size_t open = function_name_(brace.header, name);
        if (open == std::string::npos || name == "main") continue;
        std::vector<std::string> pointers;
        for (const std::string& parameter: parameters_(brace.header, open)) {
            bool pointer = false;
            const std::string parameterName = parameter_name_(parameter, pointer);
            if (pointer && !parameterName.empty()) pointers.push_back(parameterName);
        }
        if (pointers.empty()) continue;

        // Each pointer: tested (if / while / assert / ?:) before its first dereference (p->, p[, *p, p() of a function pointer)
        std::vector<std::string> missing;
        for (const std::string& pointer: pointers) {
            const std::regex dereference("\\b" + pointer + "\\s*(->|\\[|\\()|(^|[^\\w)\\]\\s]|\\breturn)\\s*\\*\\s*" + pointer + "\\b");
            const std::regex word("\\b" + pointer + "\\b");
            for (std::size_t i = brace.openLine + 1; i < brace.closeLine; ++i) {
                std::smatch use;
                std::smatch tested;
                const bool test = std::regex_search(code[i], condition) || info[i].parenDepth > 0;
                const bool dereferenced = std::regex_search(code[i], use, dereference);
                if (test && std::regex_search(code[i], tested, word) && (!dereferenced || tested.position(0) < use.position(0))) break;
                if (!dereferenced) continue;
                missing.push_back(pointer);
                break;
            }
        }
        if (missing.empty()) continue;
        std::string list;
        std::string test;
        for (const std::string& pointer: missing) {
            list += (list.empty() ? "" : ", ") + pointer;
            test += (test.empty() ? "!" : " || !") + pointer;
        }
        const std::size_t line = brace.headerLine == NO_INDEX ? brace.openLine : brace.headerLine;
        issues.push_back(xstyle::check::make_issue(file, line, 0, "C-PTR-CHECK", "Pointer parameter" + std::string(missing.size() > 1 ? "s " : " ") + list + " of " + name + " dereferenced before being checked",
            "check it first: if (" + test + ") return KO;"));
    }
}

_hot static void sizes_(const xstyle::SourceFile& file, xstyle::check::Issues& issues)
{
    const std::vector<std::string>& code = file.getCode();
    const std::vector<xstyle::Brace>& braces = file.getBraces();

    for (std::size_t b = 0; b < braces.size(); ++b) {
        const xstyle::Brace& brace = braces[b];

        // Length: the lines of code of the body (no empty line, no comment)
        if (brace.kind == xstyle::Scope::Function && brace.closeLine != NO_INDEX) {
            std::size_t lines = 0;
            for (std::size_t i = brace.openLine + 1; i < brace.closeLine; ++i)
                if (!xstyle::is_blank(code[i])) ++lines;
            std::string name;
            (void)function_name_(brace.header, name);
            if (lines > C_MAX_FUNCTION_LINES)
                issues.push_back(xstyle::check::make_issue(file, brace.headerLine == NO_INDEX ? brace.openLine : brace.headerLine, 0, "C-FUNC-LENGTH",
                    "Function " + name + ": " + std::to_string(lines) + " lines of code (max " + std::to_string(C_MAX_FUNCTION_LINES) + ")", "split it in smaller functions"));
        }

        // Nesting: the control blocks (if / for / while / switch / do / else) around, up to the function (a bare { } is not a level)
        if (brace.kind != xstyle::Scope::Control) continue;
        std::size_t depth = 0;
        for (std::size_t p = b; p != NO_INDEX && braces[p].kind != xstyle::Scope::Function; p = braces[p].parent)
            if (braces[p].kind == xstyle::Scope::Control) ++depth;
        if (depth == C_MAX_NESTING + 1) // the first level too deep only (its content is the same problem)
            issues.push_back(xstyle::check::make_issue(file, brace.headerLine == NO_INDEX ? brace.openLine : brace.headerLine, code[brace.openLine].find_first_not_of(' '),
                "C-NESTING", "Control block nested " + std::to_string(depth) + " levels deep (max " + std::to_string(C_MAX_NESTING) + ")",
                "early returns, or move the inner loop / condition to a function"));
    }
}

/* checks */
_hot void xstyle::check::c(const xstyle::SourceFile& file, xstyle::check::Issues& issues)
{
    types_(file, issues);
    names_(file, issues);
    if (!file.isHeader()) pointers_(file, issues);
    sizes_(file, issues);
}
