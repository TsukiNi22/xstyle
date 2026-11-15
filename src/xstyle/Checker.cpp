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
#include "xstyle/Rules.hpp"
#include "xstyle/Tools.hpp"
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <regex>

/* selection */
_hot bool xstyle::Checker::selected_(std::string_view code, const bool fix) const
{
    const std::vector<std::string>& codes = fix && !this->_options.fixCodes.empty() ? this->_options.fixCodes : this->_options.codes;

    for (const std::string& pattern: this->_options.ignored)
        if (xstyle::match_code(code, pattern)) return false;
    if (fix && !this->_options.fixCodes.empty() && !this->_options.codes.empty()
        && std::none_of(this->_options.codes.begin(), this->_options.codes.end(), [&](const std::string& p) {return xstyle::match_code(code, p);})) return false;
    if (codes.empty()) return true;
    return std::any_of(codes.begin(), codes.end(), [&](const std::string& p) {return xstyle::match_code(code, p);});
}

_hot bool xstyle::Checker::suppressed_(const std::vector<xstyle::Directive>& directives, const xstyle::Issue& issue) const
{
    for (const xstyle::Directive& directive: directives) {
        if (directive.kind == xstyle::DirectiveKind::On) continue;
        if (directive.first != 0 && (issue.line < directive.first || issue.line > directive.last)) continue;
        if (directive.codes.empty() || std::any_of(directive.codes.begin(), directive.codes.end(), [&](const std::string& p) {return xstyle::match_code(issue.code, p);})) return true;
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
        const std::string tail = text[i].substr(at);
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

_hot bool xstyle::Checker::skipped(const xstyle::SourceFile& file) const
{
    if (!this->_options.inlineIgnores) return false;
    const std::vector<xstyle::Directive> found = xstyle::Checker::directives(file);
    return std::any_of(found.begin(), found.end(), [](const xstyle::Directive& d) {return d.kind == xstyle::DirectiveKind::SkipFile;});
}

_hot std::vector<xstyle::Issue> xstyle::Checker::run_(const xstyle::SourceFile& file, const bool fix) const
{
    xstyle::check::Issues issues;
    const xstyle::Language language = file.getLanguage();

    // A template ({{NAME}} placeholders) is not valid code yet: only the generic rules
    static const std::regex placeholder(R"(\{\{[A-Z][A-Z0-9_]*\}\})");
    const bool templateFile = std::any_of(file.getLines().begin(), file.getLines().end(), [](const std::string& line) {
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

    // Selection: language of the rule, codes, severity, suppression comments
    const std::vector<xstyle::Directive> directives = this->_options.inlineIgnores ? xstyle::Checker::directives(file) : std::vector<xstyle::Directive>();
    std::erase_if(issues, [&](const xstyle::Issue& issue) {
        const xstyle::Rule* rule = xstyle::find_rule(issue.code);
        return !rule || !xstyle::rule_applies(*rule, language) || !this->selected_(issue.code, fix) || issue.severity < this->_options.minSeverity
            || (!this->_options.modes.empty() && std::find(this->_options.modes.begin(), this->_options.modes.end(), issue.mode) == this->_options.modes.end())
            || !this->changed_(file, issue.line)
            || this->suppressed_(directives, issue);
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
xstyle::Checker::Checker(const xstyle::Options& options, const xstyle::ProjectInfo& project)
: _options(options), _project(project)
{
    if (this->_options.libutils == "on") this->_libutils = true;
    else if (this->_options.libutils == "off") this->_libutils = false;
    else this->_libutils = this->_project.libutilsInstalled && this->_project.libutilsUsed;
}
