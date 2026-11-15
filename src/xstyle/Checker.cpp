/**************************************************************\
Edition:
##  @date 06/10/2026 by @author Tsukini

File Name:
##  @file Checker.cpp

File Description:
##  You know, I don t think there are good or bad descriptions,
##  for me, life is all about functions...
\**************************************************************/

#define _Attribute
#include <utils/utils.hpp>
#include "xstyle/check/Checks.hpp"
#include "xstyle/Checker.hpp"
#include "xstyle/Ignore.hpp"
#include "xstyle/Rules.hpp"
#include "xstyle/Tools.hpp"
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <optional>
#include <cctype>
#include <regex>

/* selection */
_hot bool xstyle::Checker::chosen_(std::string_view code, const bool fix) const
{
    // -c / --fix CODES (without -i)
    const std::vector<std::string>& codes = fix && !this->_options.fixCodes.empty() ? this->_options.fixCodes : this->_options.codes;

    if (fix && !this->_options.fixCodes.empty() && !this->_options.codes.empty()
        && std::none_of(this->_options.codes.begin(), this->_options.codes.end(), [&](const std::string& p) {return xstyle::match_code(code, p);})) return false;
    if (codes.empty()) return true;
    return std::any_of(codes.begin(), codes.end(), [&](const std::string& p) {return xstyle::match_code(code, p);});
}

_hot bool xstyle::Checker::selected_(std::string_view code, const bool fix) const
{
    for (const std::string& pattern: this->_options.ignored)
        if (xstyle::match_code(code, pattern)) return false;
    return this->chosen_(code, fix);
}

_hot bool xstyle::Checker::suppressed_(const std::vector<xstyle::Directive>& directives, const xstyle::Issue& issue) const
{
    // G-UNUSED-IGNORE only by a comment that names it (a comment silencing every rule would silence its own issue)
    const bool unused = issue.code == "G-UNUSED-IGNORE";
    for (const xstyle::Directive& directive: directives) {
        if (directive.kind == xstyle::DirectiveKind::On) continue;
        if (directive.first != 0 && (issue.line < directive.first || issue.line > directive.last)) continue;
        if ((directive.codes.empty() && !unused) || std::any_of(directive.codes.begin(), directive.codes.end(), [&](const std::string& p) {
            return unused ? p.starts_with("G-UNUSED") : xstyle::match_code(issue.code, p);
        })) return true;
    }
    return false;
}

/* directives */
_hot std::vector<xstyle::Directive> xstyle::Checker::directives(const xstyle::SourceFile& file)
{
    // xstyle: <kind> [N] [CODE, ...] [reason] in a comment (Markdown: in a <!-- --> comment, JSON: anywhere)
    static const std::regex directive(R"(xstyle:\s*(ignore-file|ignore-next|ignore|skip-file|off|on)(?![\w-])([A-Z0-9*,\s-]*))");
    static const std::regex spaces(R"(\s+)");
    static const std::regex code(R"(^(\*|[A-Z][A-Z0-9]*(-[A-Z0-9]+)*-?\*?)$)");
    static const std::unordered_map<std::string, xstyle::DirectiveKind> kinds = {
        {"ignore", xstyle::DirectiveKind::Ignore}, {"ignore-next", xstyle::DirectiveKind::IgnoreNext}, {"ignore-file", xstyle::DirectiveKind::IgnoreFile},
        {"skip-file", xstyle::DirectiveKind::SkipFile}, {"off", xstyle::DirectiveKind::Off}, {"on", xstyle::DirectiveKind::On},
    };
    const xstyle::Language language = file.getLanguage();
    const bool masked = language != xstyle::Language::Json && language != xstyle::Language::Markdown && language != xstyle::Language::Other;
    const std::vector<std::string>& text = masked ? file.getComments() : file.getLines();
    const std::size_t end = file.getLines().size();
    std::vector<xstyle::Directive> directives;

    for (std::size_t i = 0; i < text.size(); ++i) {
        const std::size_t at = text[i].find("xstyle:");
        if (at == std::string::npos) continue;
        if (language == xstyle::Language::Markdown && text[i].rfind("<!--", at) == std::string::npos) continue;
        std::smatch match;
        std::string tail = text[i].substr(at);
        tail = tail.substr(0, std::min(tail.find("*/"), tail.find("-->"))); // the end of a block comment is not a * pattern
        if (!std::regex_search(tail, match, directive) || match.position(0) != 0) continue;
        xstyle::Directive found{kinds.at(match[1].str()), i + 1, {}, i + 1, i + 1};
        std::size_t count = 1;
        for (const std::string& part: xstyle::split(std::regex_replace(match[2].str(), spaces, ","), ',')) {
            if (found.kind == xstyle::DirectiveKind::IgnoreNext && found.codes.empty() && part.size() <= 6
                && std::all_of(part.begin(), part.end(), [](const char c) {return std::isdigit(static_cast<unsigned char>(c));})) {
                count = std::max<std::size_t>(static_cast<std::size_t>(std::stoul(part)), 1);
                continue;
            }
            if (std::regex_match(part, code)) found.codes.push_back(part);
        }
        if (found.kind == xstyle::DirectiveKind::IgnoreNext) {
            found.first = i + 2;
            found.last = i + 1 + count;
        }
        if (found.kind == xstyle::DirectiveKind::IgnoreFile || found.kind == xstyle::DirectiveKind::SkipFile) found.first = found.last = 0;
        if (found.kind == xstyle::DirectiveKind::Off) found.last = end;
        if (found.kind == xstyle::DirectiveKind::On) // closes the open regions (of its codes): they end on this line
            for (xstyle::Directive& open: directives)
                if (open.kind == xstyle::DirectiveKind::Off && open.last == end && (found.codes.empty() || open.codes == found.codes)) open.last = i + 1;
        directives.push_back(found);
    }
    return directives;
}

_hot static std::optional<xstyle::Fix> remove_directive_(const xstyle::SourceFile& file, const std::size_t line)
{
    // Only a comment that is the directive alone, up to the end of the line: the whole line, or the comment after the code
    static const std::regex alone(R"(^(//+|#+|;+|--|/\*+|<!--)\s*xstyle:.*?(\*/|-->)?\s*$)");
    const std::string& text = file.getLines()[line];
    const std::string& comment = file.getLanguage() == xstyle::Language::Markdown || file.getLanguage() == xstyle::Language::Json ? text : file.getComments()[line];
    const std::size_t start = comment.find_first_not_of(" \t");
    if (start == std::string::npos) return std::nullopt;
    std::smatch match;
    const std::string rest = text.substr(start);
    if (!std::regex_match(rest, match, alone)) return std::nullopt;
    const bool block = match[1].str().starts_with("/*") || match[1].str() == "<!--";
    if (block != match[2].matched) return std::nullopt;
    const std::string code = text.substr(0, start);
    if (xstyle::is_blank(code)) return xstyle::Fix{xstyle::FixKind::Replace, line, 1, {}};
    return xstyle::Fix{xstyle::FixKind::Replace, line, 1, {code.substr(0, code.find_last_not_of(" \t") + 1)}};
}

_hot void xstyle::Checker::unused_(const xstyle::SourceFile& file, const std::vector<xstyle::Directive>& directives, xstyle::check::Issues& issues) const
{
    // A suppression comment that matches none of the issues found (before the selection: -c / -s / --diff never make it unused)
    const xstyle::Language language = file.getLanguage();
    const xstyle::check::Issues found = issues;
    for (const xstyle::Directive& directive: directives) {
        if (directive.kind == xstyle::DirectiveKind::On || directive.kind == xstyle::DirectiveKind::SkipFile) continue;
        // Never: the comments that silence this rule itself, R34 (not per file), the LU- rules when they are off
        const bool exempt = std::any_of(directive.codes.begin(), directive.codes.end(), [&](const std::string& p) {
            return p.starts_with("G-UNUSED") || p == "R34" || (!this->_libutils && p.starts_with("LU"));
        });
        if (exempt) continue;
        const bool used = std::any_of(found.begin(), found.end(), [&](const xstyle::Issue& issue) {
            const xstyle::Rule* rule = xstyle::find_rule(issue.code);
            if (!rule || !xstyle::rule_applies(*rule, language)) return false;
            if (directive.first != 0 && (issue.line < directive.first || issue.line > directive.last)) return false;
            return directive.codes.empty() || std::any_of(directive.codes.begin(), directive.codes.end(), [&](const std::string& p) {return xstyle::match_code(issue.code, p);});
        });
        if (used) continue;
        const std::string what = directive.codes.empty() ? "every rule" : [&]() {
            std::string list;
            for (const std::string& code: directive.codes)
                list += (list.empty() ? "" : ",") + code;
            return list;
        }();
        // The fix removes the comment (an off region: its closing on too, the lines between kept)
        const std::size_t line = directive.line - 1;
        std::optional<xstyle::Fix> fix = remove_directive_(file, line);
        const bool closed = directive.kind == xstyle::DirectiveKind::Off && directive.last != line + 1 && directive.last <= file.getLines().size()
            && std::any_of(directives.begin(), directives.end(), [&](const xstyle::Directive& d) {return d.kind == xstyle::DirectiveKind::On && d.line == directive.last;});
        if (fix && closed) {
            const std::optional<xstyle::Fix> end = remove_directive_(file, directive.last - 1);
            if (end) {
                std::vector<std::string> lines = fix->lines;
                lines.insert(lines.end(), file.getLines().begin() + static_cast<std::ptrdiff_t>(line + 1), file.getLines().begin() + static_cast<std::ptrdiff_t>(directive.last - 1));
                lines.insert(lines.end(), end->lines.begin(), end->lines.end());
                fix = xstyle::Fix{xstyle::FixKind::Replace, line, directive.last - line, lines};
            } else {
                fix = std::nullopt;
            }
        }
        const std::string& comments = file.getLanguage() == xstyle::Language::Markdown || file.getLanguage() == xstyle::Language::Json ? file.getLines()[line] : file.getComments()[line];
        const std::size_t column = comments.find_first_not_of(" \t");
        issues.push_back(xstyle::check::make_issue(file, line, column == std::string::npos ? 0 : column, "G-UNUSED-IGNORE", "Suppression comment of " + what + " that silences nothing",
            "remove it (the issue it silenced is gone)", fix));
    }
}

_hot bool xstyle::Checker::skipped(const xstyle::SourceFile& file) const
{
    if (!this->_options.inlineIgnores) return false;
    const std::vector<xstyle::Directive> found = xstyle::Checker::directives(file);
    return std::any_of(found.begin(), found.end(), [](const xstyle::Directive& d) {return d.kind == xstyle::DirectiveKind::SkipFile;});
}

_hot std::vector<xstyle::Issue> xstyle::Checker::raw_(const xstyle::SourceFile& file, bool& templateFile) const
{
    // Every issue of the file, before any selection
    xstyle::check::Issues issues;
    const xstyle::Language language = file.getLanguage();

    // A template ({{NAME}} placeholders) is not valid code yet: only the generic rules
    static const std::regex placeholder(R"(\{\{[A-Z][A-Z0-9_]*\}\})");
    templateFile = std::any_of(file.getLines().begin(), file.getLines().end(), [](const std::string& line) {
        return line.find("{{") != std::string::npos && std::regex_search(line, placeholder);
    });

    xstyle::check::generic(file, issues);
    if (!templateFile) {
        if (file.isCLike()) xstyle::check::cpp(file, this->_project, issues);
        if (language == xstyle::Language::C) xstyle::check::c(file, issues);
        if (language == xstyle::Language::Asm) xstyle::check::assembly(file, issues);
        if (language == xstyle::Language::Cpp && this->_libutils) xstyle::check::libutils(file, this->_project, issues);
        if (language == xstyle::Language::Python) xstyle::check::python(file, issues);
        if (language == xstyle::Language::Shell) xstyle::check::shell(file, issues);
        if (language == xstyle::Language::Rust) xstyle::check::rust(file, issues);
        if (language == xstyle::Language::CMake) xstyle::check::cmake(file, issues);
    }
    return issues;
}

_hot std::vector<std::size_t> xstyle::Checker::unused(const xstyle::SourceFile& file) const
{
    bool templateFile = false;
    std::vector<xstyle::Issue> issues = this->raw_(file, templateFile);
    std::vector<std::size_t> lines;
    if (templateFile) return lines;
    const std::size_t count = issues.size();
    this->unused_(file, xstyle::Checker::directives(file), issues);
    for (std::size_t i = count; i < issues.size(); ++i)
        lines.push_back(issues[i].line);
    return lines;
}

_hot std::vector<xstyle::Issue> xstyle::Checker::run_(const xstyle::SourceFile& file, const bool fix, std::vector<xstyle::Issue>* ignored) const
{
    bool templateFile = false;
    xstyle::check::Issues issues = this->raw_(file, templateFile);
    const xstyle::Language language = file.getLanguage();

    // Selection: language of the rule, codes, severity, changed lines; then what is ignored (counted): comments (and the ones that silence
    // nothing), rules ignored by path (.xignore, [[path]] of .xconfig, --ignore-path), rules ignored (-i)
    const std::vector<xstyle::Directive> directives = this->_options.inlineIgnores ? xstyle::Checker::directives(file) : std::vector<xstyle::Directive>();
    const std::vector<std::string> pathCodes = this->_ignore ? this->_ignore->codes(file.getPath()) : std::vector<std::string>();
    if (!templateFile) this->unused_(file, directives, issues);
    std::erase_if(issues, [&](xstyle::Issue& issue) {
        const xstyle::Rule* rule = xstyle::find_rule(issue.code);
        if (!rule || !xstyle::rule_applies(*rule, language) || !this->chosen_(issue.code, fix) || issue.severity < this->_options.minSeverity
            || (!this->_options.modes.empty() && std::find(this->_options.modes.begin(), this->_options.modes.end(), issue.mode) == this->_options.modes.end())
            || !this->changed_(file, issue.line)) return true;
        const std::function<bool(const std::vector<std::string>&)> any = [&](const std::vector<std::string>& patterns) {
            return std::any_of(patterns.begin(), patterns.end(), [&](const std::string& p) {return xstyle::match_code(issue.code, p);});
        };
        if (this->suppressed_(directives, issue)) issue.ignoredBy = "comment";
        else if (any(pathCodes)) issue.ignoredBy = "path";
        else if (any(this->_options.ignored)) issue.ignoredBy = "rule";
        else return false;
        if (ignored) ignored->push_back(issue);
        return true;
    });
    std::stable_sort(issues.begin(), issues.end(), [](const xstyle::Issue& a, const xstyle::Issue& b) {return a.line != b.line ? a.line < b.line : a.column < b.column;});
    return issues;
}

_hot bool xstyle::Checker::changed_(const xstyle::SourceFile& file, const std::size_t line) const
{
    // --diff / --staged: only the issues on a changed line (whole file issues: the file changed)
    if (!this->_options.changedOnly) return true;
    std::error_code error;
    auto it = this->_options.changedLines.find(std::filesystem::weakly_canonical(file.getPath(), error));
    if (it == this->_options.changedLines.end()) return false;
    if (line == 0) return true;
    return std::any_of(it->second.begin(), it->second.end(), [&](const std::pair<std::size_t, std::size_t>& range) {return line >= range.first && line <= range.second;});
}

/* fix */
_hot std::vector<xstyle::AppliedFix> xstyle::Checker::fix(xstyle::SourceFile& file) const
{
    std::vector<xstyle::AppliedFix> applied;

    for (std::size_t pass = 0; pass < FIX_PASSES; ++pass) {
        // Non-overlapping fixes of this pass (the others come back on the next pass)
        std::vector<xstyle::Fix> fixes;
        std::vector<std::pair<std::size_t, std::size_t>> ranges;
        for (const xstyle::Issue& issue: this->run_(file, true)) {
            if (!issue.fix || (issue.fix->unsafe && !this->_options.force) || (issue.fix->dangerous && !this->_options.dangerous)) continue;
            const xstyle::Fix& fix = *issue.fix;
            xstyle::AppliedFix record{issue.code, issue.line, {}, fix.lines};
            if (fix.kind != xstyle::FixKind::Replace) {
                record.before = {fix.kind == xstyle::FixKind::Crlf ? "(CRLF line endings)" : "(no final newline)"};
                record.after = {fix.kind == xstyle::FixKind::Crlf ? "(LF line endings)" : "(final newline)"};
                fixes.push_back(fix);
                applied.push_back(record);
                continue;
            }
            const std::size_t first = fix.line;
            const std::size_t last = fix.line + std::max<std::size_t>(fix.count, 1);
            if (std::any_of(ranges.begin(), ranges.end(), [&](const std::pair<std::size_t, std::size_t>& r) {return first < r.second + 1 && r.first < last + 1;})) continue;
            const std::vector<std::string>& lines = file.getLines();
            record.before.assign(lines.begin() + static_cast<std::ptrdiff_t>(std::min(first, lines.size())), lines.begin() + static_cast<std::ptrdiff_t>(std::min(first + fix.count, lines.size())));
            if (record.before == record.after) continue; // no-op: would loop forever
            ranges.push_back({first, last});
            fixes.push_back(fix);
            applied.push_back(record);
        }
        if (fixes.empty()) break;
        file.apply(fixes);
    }
    return applied;
}

/* constructor */
xstyle::Checker::Checker(const xstyle::Options& options, const xstyle::ProjectInfo& project, const xstyle::Ignore* ignore)
: _options(options), _project(project), _ignore(ignore)
{
    if (this->_options.libutils == "on") this->_libutils = true;
    else if (this->_options.libutils == "off") this->_libutils = false;
    else this->_libutils = this->_project.libutilsInstalled && this->_project.libutilsUsed;
}
