/**************************************************************\
Edition:
##  @date 13/11/2026 by @author Tsukini

File Name:
##  @file asm.cpp

File Description:
##  Rules of the assembly (NASM, x86-64): layout, names,
##  declarations, functions (ABI, stack frame, fall through), syscalls.
\**************************************************************/

#define _Attribute
#include <utils/utils.hpp>
#include "xstyle/check/Checks.hpp"
#include "xstyle/Tools.hpp"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <cctype>
#include <regex>
#include <map>
#include <set>

// What a line holds
enum class Kind {
    Blank,
    Comment, // comment only
    Section,
    Directive, // global / extern / bits / default / align...
    Preprocessor, // %define / %macro / %if...
    Data, // db / resb / equ / times (with or without its label)
    Code, // a label and / or an instruction
};

// A line of the file, parsed from its masked code
struct AsmLine {
    Kind kind = Kind::Blank;
    std::string label; // label of the line (name: / name db ...), empty if none
    std::size_t labelColumn = 0;
    std::string mnemonic; // instruction (lower case), prefix rep / lock skipped
    std::size_t mnemonicColumn = 0;
    std::vector<std::string> operands;
    std::string word; // first word of a directive / preprocessor line (lower case)
    std::string argument; // rest of a directive line (trimmed)
    bool comment = false; // has a trailing comment after the code
    std::size_t codeEnd = 0; // end of the code (trailing spaces excluded)
    std::size_t commentColumn = 0;
};

// A function: a code label (not a local one, not a jump label) up to the next one
struct AsmFunction {
    std::string name;
    std::size_t start = 0; // line of its label
    std::size_t end = 0; // first line after it
    bool global = false;
};

/* tools */
_hot static std::string lower_(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) {return static_cast<char>(std::tolower(c));});
    return s;
}

_hot static std::string family_(const std::string& operand) // 64-bit register of a general register (eax -> rax), empty if not one
{
    static const std::unordered_map<std::string, std::string> families = [] {
        std::unordered_map<std::string, std::string> map;
        const std::vector<std::pair<std::string, std::vector<std::string>>> legacy = {
            {"rax", {"eax", "ax", "al", "ah"}}, {"rbx", {"ebx", "bx", "bl", "bh"}}, {"rcx", {"ecx", "cx", "cl", "ch"}}, {"rdx", {"edx", "dx", "dl", "dh"}},
            {"rsi", {"esi", "si", "sil"}}, {"rdi", {"edi", "di", "dil"}}, {"rbp", {"ebp", "bp", "bpl"}}, {"rsp", {"esp", "sp", "spl"}}};
        for (const auto &[full, parts]: legacy) {
            map[full] = full;
            for (const std::string& part: parts)
                map[part] = full;
        }
        for (int n = 8; n <= 15; ++n) {
            const std::string full = "r" + std::to_string(n);
            for (const std::string& suffix: {"", "d", "w", "b", "l"})
                map[full + suffix] = full;
        }
        return map;
    }();
    const std::string key = lower_(xstyle::trim(operand));
    return families.contains(key) ? families.at(key) : "";
}

_hot static bool register_name_(const std::string& word) // any register (general, segment, vector, rip...)
{
    static const std::regex other(R"(^(?:[xyz]mm[0-9]+|k[0-7]|st[0-7]?|mm[0-7]|[c-gs]s|rip|eip|ip|cr[0-8]|dr[0-7])$)");
    return !family_(word).empty() || std::regex_match(lower_(word), other);
}

_hot static std::vector<std::string> registers_in_(const std::string& operand) // general registers read through an operand ([rdi + 8] -> rdi)
{
    static const std::regex word(R"([A-Za-z_]\w*)");
    std::vector<std::string> found;
    for (std::sregex_iterator it(operand.begin(), operand.end(), word); it != std::sregex_iterator(); ++it) {
        const std::string family = family_((*it)[0].str());
        if (!family.empty()) found.push_back(family);
    }
    return found;
}

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
        if (std::isupper(c) && i > 0 && std::islower(static_cast<unsigned char>(name[i - 1]))) snake += '_';
        snake += static_cast<char>(std::tolower(c));
    }
    return snake;
}

_hot static bool jump_(const std::string& mnemonic)
{
    return (mnemonic.starts_with("j") && mnemonic != "jmp") || mnemonic == "jmp" || mnemonic.starts_with("loop");
}

_hot static bool terminal_(const std::string& mnemonic) // never continues to the next line
{
    static const std::unordered_set<std::string> words = {"ret", "retn", "retf", "retq", "iret", "iretd", "iretq", "jmp", "hlt", "ud2", "sysret", "sysexit"};
    return words.contains(mnemonic);
}

_hot static std::string syscall_name_(const long number) // x86-64 Linux
{
    static const std::map<long, std::string> names = {
        {0, "read"}, {1, "write"}, {2, "open"}, {3, "close"}, {4, "stat"}, {5, "fstat"}, {6, "lstat"}, {7, "poll"}, {8, "lseek"}, {9, "mmap"}, {10, "mprotect"},
        {11, "munmap"}, {12, "brk"}, {13, "rt_sigaction"}, {14, "rt_sigprocmask"}, {15, "rt_sigreturn"}, {16, "ioctl"}, {17, "pread64"}, {18, "pwrite64"},
        {19, "readv"}, {20, "writev"}, {21, "access"}, {22, "pipe"}, {23, "select"}, {24, "sched_yield"}, {25, "mremap"}, {26, "msync"}, {27, "mincore"},
        {28, "madvise"}, {29, "shmget"}, {30, "shmat"}, {31, "shmctl"}, {32, "dup"}, {33, "dup2"}, {34, "pause"}, {35, "nanosleep"}, {36, "getitimer"},
        {37, "alarm"}, {38, "setitimer"}, {39, "getpid"}, {40, "sendfile"}, {41, "socket"}, {42, "connect"}, {43, "accept"}, {44, "sendto"}, {45, "recvfrom"},
        {46, "sendmsg"}, {47, "recvmsg"}, {48, "shutdown"}, {49, "bind"}, {50, "listen"}, {51, "getsockname"}, {52, "getpeername"}, {53, "socketpair"},
        {54, "setsockopt"}, {55, "getsockopt"}, {56, "clone"}, {57, "fork"}, {58, "vfork"}, {59, "execve"}, {60, "exit"}, {61, "wait4"}, {62, "kill"},
        {63, "uname"}, {72, "fcntl"}, {73, "flock"}, {74, "fsync"}, {76, "truncate"}, {77, "ftruncate"}, {78, "getdents"}, {79, "getcwd"}, {80, "chdir"},
        {81, "fchdir"}, {82, "rename"}, {83, "mkdir"}, {84, "rmdir"}, {85, "creat"}, {86, "link"}, {87, "unlink"}, {88, "symlink"}, {89, "readlink"},
        {90, "chmod"}, {91, "fchmod"}, {92, "chown"}, {95, "umask"}, {96, "gettimeofday"}, {97, "getrlimit"}, {102, "getuid"}, {104, "getgid"}, {105, "setuid"},
        {106, "setgid"}, {107, "geteuid"}, {108, "getegid"}, {110, "getppid"}, {186, "gettid"}, {201, "time"}, {202, "futex"}, {217, "getdents64"},
        {228, "clock_gettime"}, {230, "clock_nanosleep"}, {231, "exit_group"}, {257, "openat"}, {262, "newfstatat"}, {293, "pipe2"}, {318, "getrandom"},
        {319, "memfd_create"}, {322, "execveat"}};
    return names.contains(number) ? names.at(number) : "";
}

_hot static bool number_(const std::string& text, long& value)
{
    static const std::regex decimal(R"(^[0-9]+$)");
    static const std::regex hexadecimal(R"(^0x[0-9a-fA-F]+$|^[0-9][0-9a-fA-F]*h$)");
    const std::string t = xstyle::trim(text);
    if (std::regex_match(t, decimal)) value = std::stol(t);
    else if (std::regex_match(t, hexadecimal)) value = std::stol(t.starts_with("0x") ? t.substr(2) : t.substr(0, t.size() - 1), nullptr, 16);
    else return false;
    return true;
}

/* parse */
_hot static std::vector<AsmLine> parse_(const xstyle::SourceFile& file)
{
    static const std::unordered_set<std::string> directives = {"section", "segment", "global", "extern", "bits", "default", "align", "alignb", "cpu", "org",
        "use16", "use32", "use64", "common", "static", "absolute", "struc", "endstruc", "istruc", "iend", "at"};
    static const std::unordered_set<std::string> data = {"db", "dw", "dd", "dq", "dt", "do", "dy", "dz", "resb", "resw", "resd", "resq", "rest", "reso", "resy",
        "resz", "incbin", "times", "equ"};
    static const std::unordered_set<std::string> prefixes = {"rep", "repe", "repz", "repne", "repnz", "lock"};
    static const std::regex label(R"(^\s*([A-Za-z_.?$@][\w.?$@#~]*)\s*:(?!:))");
    static const std::regex word(R"(^\s*([%A-Za-z_.?$@][\w.?$@#~]*))");
    const std::vector<std::string>& code = file.getCode();
    const std::vector<std::string>& comments = file.getComments();
    std::vector<AsmLine> lines(code.size());

    for (std::size_t i = 0; i < code.size(); ++i) {
        AsmLine& line = lines[i];
        std::string text = code[i];
        const std::size_t end = text.find_last_not_of(' ');
        line.codeEnd = end == std::string::npos ? 0 : end + 1;
        line.comment = !xstyle::is_blank(comments[i]) && line.codeEnd > 0;
        line.commentColumn = comments[i].find_first_not_of(' ');
        if (xstyle::is_blank(text)) {
            line.kind = xstyle::is_blank(comments[i]) ? Kind::Blank : Kind::Comment;
            continue;
        }
        if (xstyle::trim(text).starts_with("[")) { // [section .text] / [bits 64]
            const std::string inner = xstyle::trim(text).substr(1);
            line.word = lower_(inner.substr(0, inner.find_first_of(" ]")));
            line.argument = xstyle::trim(inner.substr(line.word.size(), inner.find(']') == std::string::npos ? std::string::npos : inner.find(']') - line.word.size()));
            line.kind = line.word == "section" || line.word == "segment" ? Kind::Section : Kind::Directive;
            continue;
        }

        std::smatch match;
        std::size_t position = 0;
        if (std::regex_search(text, match, label)) {
            line.label = match[1].str();
            line.labelColumn = static_cast<std::size_t>(match.position(1));
            position = static_cast<std::size_t>(match.position(0) + match.length(0));
            line.kind = Kind::Code;
        }
        std::string rest = text.substr(position);
        if (!std::regex_search(rest, match, word)) continue;
        std::string first = match[1].str();
        std::size_t firstColumn = position + static_cast<std::size_t>(match.position(1));
        std::string after = rest.substr(static_cast<std::size_t>(match.position(1) + match.length(1)));

        if (first.starts_with("%")) {
            line.kind = Kind::Preprocessor;
            line.word = lower_(first);
            line.argument = xstyle::trim(after);
            continue;
        }
        if (line.label.empty() && directives.contains(lower_(first))) {
            line.word = lower_(first);
            line.argument = xstyle::trim(after);
            line.kind = line.word == "section" || line.word == "segment" ? Kind::Section : Kind::Directive;
            continue;
        }
        // name db ... (a data label without its colon)
        std::smatch second;
        if (line.label.empty() && std::regex_search(after, second, word) && data.contains(lower_(second[1].str()))) {
            line.label = first;
            line.labelColumn = firstColumn;
            line.kind = Kind::Data;
            line.word = lower_(second[1].str());
            line.argument = xstyle::trim(after.substr(static_cast<std::size_t>(second.position(1) + second.length(1))));
            continue;
        }
        if (data.contains(lower_(first))) {
            line.kind = Kind::Data;
            line.word = lower_(first);
            line.argument = xstyle::trim(after);
            continue;
        }

        // Instruction (rep / lock prefix skipped)
        if (prefixes.contains(lower_(first)) && std::regex_search(after, second, word)) {
            firstColumn += static_cast<std::size_t>(match.length(1)) + static_cast<std::size_t>(second.position(1));
            first = second[1].str();
            after = after.substr(static_cast<std::size_t>(second.position(1) + second.length(1)));
        }
        line.kind = Kind::Code;
        line.mnemonic = lower_(first);
        line.mnemonicColumn = firstColumn;
        std::string current;
        int depth = 0;
        for (const char c: after) {
            if (c == '[' || c == '(') ++depth;
            if (c == ']' || c == ')') --depth;
            if (c == ',' && depth == 0) {
                line.operands.push_back(xstyle::trim(current));
                current.clear();
                continue;
            }
            current += c;
        }
        if (!xstyle::trim(current).empty()) line.operands.push_back(xstyle::trim(current));
    }
    return lines;
}

/* checks: file */
_hot static void header_(const xstyle::SourceFile& file, xstyle::check::Issues& issues)
{
    static const std::regex fileTag(R"(@file\s+(\S+))");
    const std::vector<std::string>& lines = file.getLines();
    const std::string name = file.getPath().filename().string();

    std::size_t first = 0;
    while (first < lines.size() && xstyle::is_blank(lines[first])) ++first;
    if (first >= lines.size()) return;
    for (std::size_t i = first; i < std::min(lines.size(), first + 40); ++i) {
        std::smatch match;
        if (!std::regex_search(lines[i], match, fileTag)) continue;
        if (match[1].str() != name)
            issues.push_back(xstyle::check::make_issue(file, i, static_cast<std::size_t>(match.position(1)), "ASM-HEADER-FILE", "@file " + match[1].str() + " in " + name, "",
                xstyle::check::replace_line(i, lines[i].substr(0, static_cast<std::size_t>(match.position(1))) + name + lines[i].substr(static_cast<std::size_t>(match.position(1) + match.length(1))))));
        if (xstyle::trim(lines[first]).starts_with(";")) return;
        break;
    }
    xstyle::Issue issue = xstyle::check::make_issue(file, first, 0, "ASM-HEADER", "No file header",
        "--fix asks the banner (none / default / text) and the description, or --header none|default|<text> [--header-desc \"...\"]");
    issue.mode = xstyle::FixMode::Ask;
    issues.push_back(issue);
}

_hot static void sections_(const xstyle::SourceFile& file, const std::vector<AsmLine>& lines, xstyle::check::Issues& issues)
{
    static const std::vector<std::string> order = {".data", ".rodata", ".bss", ".text"};
    std::size_t rank = 0;
    std::set<std::string> seen;
    std::size_t firstSection = NO_INDEX;
    bool textLabel = false; // a label already in the text section (the global go before)
    std::unordered_map<std::string, std::size_t> globals;
    std::unordered_map<std::string, std::size_t> externs;
    std::unordered_set<std::string> labels;
    std::string section;

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const AsmLine& line = lines[i];
        if (!line.label.empty()) labels.insert(line.label);
        if (line.kind == Kind::Section) {
            section = line.argument.substr(0, line.argument.find_first_of(" \t:"));
            if (firstSection == NO_INDEX) firstSection = i;
            textLabel = false;
            const std::vector<std::string>::const_iterator it = std::find(order.begin(), order.end(), section);
            if (it == order.end() || !seen.insert(section).second) continue;
            const std::size_t current = static_cast<std::size_t>(it - order.begin());
            if (current < rank)
                issues.push_back(xstyle::check::make_issue(file, i, 0, "ASM-SECTION-ORDER", "section " + section + " after section " + order[rank], "order: .data, .rodata, .bss, .text"));
            rank = std::max(rank, current);
            continue;
        }
        if (line.kind == Kind::Code && !line.label.empty() && section.starts_with(".text")) textLabel = true;
        if (line.kind != Kind::Directive || (line.word != "global" && line.word != "extern")) continue;
        for (const std::string& symbol: xstyle::split(line.argument, ',')) {
            const std::string name = xstyle::trim(symbol.substr(0, symbol.find(':')));
            if (name.empty()) continue;
            if (line.word == "extern") {
                externs[name] = i;
                if (firstSection != NO_INDEX)
                    issues.push_back(xstyle::check::make_issue(file, i, 0, "ASM-SYMBOL-DECL", "extern " + name + " after the first section", "the extern go at the top of the file, before any section"));
                continue;
            }
            globals[name] = i;
            if (!section.starts_with(".text") || textLabel)
                issues.push_back(xstyle::check::make_issue(file, i, 0, "ASM-SYMBOL-DECL", "global " + name + " not at the start of section .text", "right after section .text, before its first label"));
        }
    }
    for (const auto &[name, line]: globals)
        if (!labels.contains(name))
            issues.push_back(xstyle::check::make_issue(file, line, 0, "ASM-SYMBOL-DECL", "global " + name + " without its label in this file", "define " + name + ": or remove the global"));
    for (const auto &[name, line]: externs) {
        bool used = false;
        const std::regex reference("(^|[^\\w.])" + name + "\\b");
        for (std::size_t i = 0; i < lines.size() && !used; ++i)
            used = i != line && std::regex_search(file.getCode()[i], reference);
        if (!used) issues.push_back(xstyle::check::make_issue(file, line, 0, "ASM-SYMBOL-DECL", "extern " + name + " never used", "remove it"));
    }
}

/* checks: layout */
_hot static void indentation_(const xstyle::SourceFile& file, const std::vector<AsmLine>& lines, xstyle::check::Issues& issues)
{
    // Section / code label: 0; section content, global / extern / instructions: 4; a local label: 4 per open block, its content one more
    // A local block ends at its last backward jump (a loop), else at the next label
    const std::vector<std::string>& text = file.getLines();
    std::string section;
    bool macro = false;
    struct Block {
        std::string label;
        std::size_t end; // NO_INDEX: up to the next label
    };
    std::vector<Block> stack;
    std::vector<std::size_t> levels(lines.size(), NO_INDEX); // expected indent of each line checked

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const AsmLine& line = lines[i];
        if (line.kind == Kind::Preprocessor && (line.word == "%macro" || line.word == "%imacro")) macro = true;
        if (line.kind == Kind::Preprocessor && line.word == "%endmacro") {
            macro = false;
            continue;
        }
        if (macro || line.kind == Kind::Blank || line.kind == Kind::Comment || line.kind == Kind::Preprocessor) continue;
        while (!stack.empty() && stack.back().end != NO_INDEX && stack.back().end < i) stack.pop_back();

        std::size_t expected = 0;
        const bool code = section.starts_with(".text") || section.empty();
        if (line.kind == Kind::Section) {
            section = line.argument.substr(0, line.argument.find_first_of(" \t:"));
            stack.clear();
        } else if (section.empty()) {
            expected = 0; // before any section: extern, constants...
        } else if (!code) {
            expected = 4;
        } else if (line.kind == Kind::Code && !line.label.empty() && !line.label.starts_with(".")) {
            stack.clear(); // a function
        } else if (line.kind == Kind::Code && !line.label.empty()) {
            while (!stack.empty() && stack.back().end == NO_INDEX) stack.pop_back(); // the forward blocks end at the next label
            expected = 4 + 4 * stack.size();
            std::size_t last = NO_INDEX; // last backward jump to it
            for (std::size_t k = i + 1; k < lines.size(); ++k) {
                if (lines[k].kind == Kind::Code && !lines[k].label.empty() && !lines[k].label.starts_with(".")) break;
                if (jump_(lines[k].mnemonic) && !lines[k].operands.empty() && xstyle::trim(lines[k].operands.back()) == line.label) last = k;
            }
            stack.push_back({line.label, last});
        } else {
            expected = 4 + 4 * stack.size();
        }

        levels[i] = expected;
    }

    // A comment alone on its line: at the level of the code under it
    for (std::size_t i = lines.size(); i-- > 0;)
        if (lines[i].kind == Kind::Comment && i + 1 < lines.size() && levels[i] == NO_INDEX) {
            std::size_t next = i + 1;
            while (next < lines.size() && (lines[next].kind == Kind::Blank || lines[next].kind == Kind::Comment) && levels[next] == NO_INDEX) ++next;
            if (next < lines.size() && (lines[next].kind != Kind::Blank || levels[next] != NO_INDEX)) levels[i] = levels[next];
        }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::size_t expected = levels[i];
        const std::size_t indent = xstyle::indentation(text[i]).size();
        if (expected == NO_INDEX || indent == expected || text[i].find('\t') < text[i].find_first_not_of(" \t")) continue;
        issues.push_back(xstyle::check::make_issue(file, i, 0, "ASM-INDENT", "Indented by " + std::to_string(indent) + " instead of " + std::to_string(expected), "",
            xstyle::check::replace_line(i, std::string(expected, ' ') + text[i].substr(std::min(text[i].size(), indent)))));
    }
}

_hot static void comments_(const xstyle::SourceFile& file, const std::vector<AsmLine>& lines, xstyle::check::Issues& issues)
{
    // Consecutive lines that all end with a comment: their ; on one column (the longest code + 1, or further when they already are)
    const std::vector<std::string>& text = file.getLines();
    // a label alone with its comment (its registers, what it does) is not part of a block
    const std::function<bool(std::size_t)> commented = [&lines](const std::size_t i) {return lines[i].comment && !(lines[i].kind == Kind::Code && lines[i].mnemonic.empty());};
    for (std::size_t i = 0; i < lines.size();) {
        if (!commented(i)) {
            ++i;
            continue;
        }
        std::size_t end = i;
        std::size_t longest = 0;
        std::set<std::size_t> columns;
        for (; end < lines.size() && commented(end); ++end) {
            longest = std::max(longest, lines[end].codeEnd);
            columns.insert(lines[end].commentColumn);
        }
        if (end - i >= 2 && (columns.size() > 1 || *columns.begin() <= longest)) {
            const std::size_t target = std::max(longest + 1, *columns.begin());
            std::vector<std::string> fixed;
            for (std::size_t k = i; k < end; ++k)
                fixed.push_back(text[k].substr(0, lines[k].codeEnd) + std::string(target - lines[k].codeEnd, ' ') + text[k].substr(lines[k].commentColumn));
            issues.push_back(xstyle::check::make_issue(file, i, lines[i].commentColumn, "ASM-COMMENT-ALIGN", "Trailing comments of a block not aligned on one column", "",
                xstyle::check::replace_lines(i, end - i, fixed)));
        }
        i = end;
    }
}

_hot static void case_(const xstyle::SourceFile& file, const std::vector<AsmLine>& lines, xstyle::check::Issues& issues)
{
    // Mnemonics, registers, sizes and directives in lower case (the constants keep their UPPER_SNAKE)
    static const std::unordered_set<std::string> sizes = {"byte", "word", "dword", "qword", "tword", "oword", "yword", "zword", "rel", "abs", "near", "far", "short", "strict"};
    static const std::regex identifier(R"([A-Za-z_]\w*)");
    const std::vector<std::string>& text = file.getLines();

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const AsmLine& line = lines[i];
        std::vector<std::pair<std::size_t, std::size_t>> words; // <column, length> to lower
        const std::string& code = file.getCode()[i];
        if (line.kind == Kind::Code && !line.mnemonic.empty() && code.compare(line.mnemonicColumn, line.mnemonic.size(), line.mnemonic) != 0)
            words.push_back({line.mnemonicColumn, line.mnemonic.size()});
        if ((line.kind == Kind::Section || line.kind == Kind::Directive || line.kind == Kind::Data) && !line.word.empty()) {
            const std::size_t at = lower_(code).find(line.word);
            if (at != std::string::npos && code.compare(at, line.word.size(), line.word) != 0) words.push_back({at, line.word.size()});
        }
        if (line.kind == Kind::Code && !line.mnemonic.empty()) {
            const std::size_t from = line.mnemonicColumn + line.mnemonic.size();
            const std::string operands = code.substr(std::min(from, code.size()));
            for (std::sregex_iterator it(operands.begin(), operands.end(), identifier); it != std::sregex_iterator(); ++it) {
                const std::string name = (*it)[0].str();
                if (name == lower_(name) || (!register_name_(name) && !sizes.contains(lower_(name)))) continue;
                words.push_back({from + static_cast<std::size_t>(it->position(0)), name.size()});
            }
        }
        if (words.empty()) continue;
        std::string fixed = text[i];
        for (const auto &[column, length]: words)
            for (std::size_t k = column; k < column + length && k < fixed.size(); ++k)
                fixed[k] = static_cast<char>(std::tolower(static_cast<unsigned char>(fixed[k])));
        issues.push_back(xstyle::check::make_issue(file, i, words.front().first, "ASM-CASE", "Upper case mnemonic / register / directive", "", xstyle::check::replace_line(i, fixed)));
    }
}

/* checks: names */
_hot static std::vector<AsmFunction> names_(const xstyle::SourceFile& file, const std::vector<AsmLine>& lines, xstyle::check::Issues& issues)
{
    static const std::regex upper(R"(^[A-Z][A-Z0-9_]*$)");
    static const std::regex local(R"(^\.\.?@?[a-z][a-z0-9_]*$)");
    static const std::regex data(R"(^[a-z][a-z0-9_]*$)");
    std::unordered_set<std::string> globals;
    std::unordered_map<std::string, std::size_t> calls; // label -> referenced by a call / a non jump instruction
    std::unordered_map<std::string, std::vector<std::size_t>> jumps; // label -> lines jumping to it
    std::vector<std::pair<std::size_t, std::string>> codeLabels; // <line, name> of the non local labels of the text sections
    std::string section;

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const AsmLine& line = lines[i];
        if (line.kind == Kind::Section) section = line.argument.substr(0, line.argument.find_first_of(" \t:"));
        if (line.kind == Kind::Directive && line.word == "global")
            for (const std::string& symbol: xstyle::split(line.argument, ','))
                globals.insert(xstyle::trim(symbol.substr(0, symbol.find(':'))));

        // Constants: name equ / %define / %assign / %macro -> UPPER_SNAKE
        std::string constant;
        if (line.kind == Kind::Data && line.word == "equ") constant = line.label;
        if (line.kind == Kind::Preprocessor && (line.word == "%define" || line.word == "%xdefine" || line.word == "%assign" || line.word == "%macro" || line.word == "%imacro"))
            constant = line.argument.substr(0, line.argument.find_first_of(" \t("));
        if (!constant.empty() && !std::regex_match(constant, upper)) {
            const std::size_t column = file.getCode()[i].find(constant);
            issues.push_back(xstyle::check::make_issue(file, i, column == std::string::npos ? 0 : column, "ASM-CONST-NAME", "Constant / macro " + constant + " not in UPPER_SNAKE",
                "rename it in UPPER_SNAKE"));
        }
        // Data labels: snake_case, never a register name
        if (line.kind == Kind::Data && line.word != "equ" && !line.label.empty() && (!std::regex_match(line.label, data) || register_name_(line.label)))
            issues.push_back(xstyle::check::make_issue(file, i, line.labelColumn, "ASM-DATA-NAME", "Data label " + line.label + (register_name_(line.label) ? " is a register name" : " not in snake_case"),
                register_name_(line.label) ? "rename it (" + line.label + "_value...)" : "rename it " + to_snake_(line.label)));

        if (line.kind == Kind::Code && !line.label.empty() && !line.label.starts_with(".") && (section.starts_with(".text") || section.empty()))
            codeLabels.push_back({i, line.label});
        if (line.mnemonic.empty()) continue;
        for (const std::string& operand: line.operands) {
            const std::string target = xstyle::trim(operand.substr(operand.find_last_of(" [") == std::string::npos ? 0 : operand.find_last_of(" [") + 1));
            const std::string name = target.substr(0, target.find_first_of("]+-*"));
            if (jump_(line.mnemonic)) jumps[name].push_back(i);
            else calls[name] = i;
        }
    }

    // Each code label: a function (called, global, referenced) or a jump label of the function above it
    std::vector<AsmFunction> functions;
    for (std::size_t k = 0; k < codeLabels.size(); ++k) {
        const auto &[line, name] = codeLabels[k];
        const bool global = globals.contains(name);
        const std::size_t next = k + 1 < codeLabels.size() ? codeLabels[k + 1].first : lines.size();
        bool jumpLabel = !global && !calls.contains(name) && jumps.contains(name) && !functions.empty();
        if (jumpLabel)
            for (const std::size_t from: jumps.at(name)) // only from the function above it or from its own lines (a loop)
                jumpLabel = jumpLabel && from > functions.back().start && from < next;
        if (jumpLabel) {
            issues.push_back(xstyle::check::make_issue(file, line, lines[line].labelColumn, "ASM-LOCAL-LABEL", "Jump label " + name + " not local", "." + to_snake_(name.starts_with("_") ? name.substr(1) : name)
                + ": (a label only jumped to from its function)"));
            continue;
        }
        if (!functions.empty()) functions.back().end = line;
        functions.push_back({name, line, lines.size(), global});
        if (name == "_start" || name == "main") continue;
        if (!snake_(name))
            issues.push_back(xstyle::check::make_issue(file, line, lines[line].labelColumn, "ASM-LABEL-NAME", "Label " + name + " not in snake_case", "rename it " + to_snake_(name)));
        else if (global && name.starts_with("_"))
            issues.push_back(xstyle::check::make_issue(file, line, lines[line].labelColumn, "ASM-LABEL-NAME", "Global function " + name + " starts with _ (kept for the static functions)",
                "rename it " + name.substr(name.find_first_not_of('_'))));
        else if (!global && !name.starts_with("_"))
            issues.push_back(xstyle::check::make_issue(file, line, lines[line].labelColumn, "ASM-LABEL-NAME", "Static function " + name + " (not global) without its _ prefix", "rename it _" + name));
    }
    // A function ends at the next section too
    for (AsmFunction& function: functions)
        for (std::size_t i = function.start + 1; i < function.end; ++i)
            if (lines[i].kind == Kind::Section) {
                function.end = i;
                break;
            }

    // Local labels: .snake_case
    for (std::size_t i = 0; i < lines.size(); ++i)
        if (lines[i].kind == Kind::Code && lines[i].label.starts_with(".") && !std::regex_match(lines[i].label, local))
            issues.push_back(xstyle::check::make_issue(file, i, lines[i].labelColumn, "ASM-LABEL-NAME", "Local label " + lines[i].label + " not in snake_case",
                "rename it ." + to_snake_(lines[i].label.substr(1))));
    return functions;
}

/* checks: functions */
_hot static void functions_(const xstyle::SourceFile& file, const std::vector<AsmLine>& lines, const std::vector<AsmFunction>& functions, xstyle::check::Issues& issues)
{
    static const std::unordered_set<std::string> writeOnly = {"mov", "movzx", "movsx", "movsxd", "movabs", "lea", "pop"};
    static const std::unordered_set<std::string> readOnly = {"cmp", "test", "push", "bt", "call", "jmp"};
    static const std::unordered_set<std::string> calleeSaved = {"rbx", "rbp", "r12", "r13", "r14", "r15"};
    const std::vector<std::string>& text = file.getLines();

    for (std::size_t f = 0; f < functions.size(); ++f) {
        const AsmFunction& function = functions[f];
        std::vector<std::size_t> body; // its instruction lines
        for (std::size_t i = function.start; i < function.end; ++i)
            if (!lines[i].mnemonic.empty()) body.push_back(i);
        if (body.empty()) continue;

        // Registers read before written (its inputs), rax written before a ret (its output), saved registers, stack use
        std::set<std::string> written;
        std::set<std::string> inputs;
        std::map<std::string, std::size_t> firstWrite;
        std::set<std::string> pushed;
        std::size_t framePush = NO_INDEX;
        bool stack = false; // another push / pop, rsp changed, [rbp / [rsp], a call
        for (const std::size_t i: body) {
            const AsmLine& line = lines[i];
            const std::string& m = line.mnemonic;
            const std::vector<std::string>& ops = line.operands;
            const std::string destination = ops.empty() ? "" : family_(ops[0]);
            const bool zeroing = (m == "xor" || m == "sub") && ops.size() == 2 && destination != "" && destination == family_(ops[1]);
            for (std::size_t o = 0; o < ops.size() && !zeroing; ++o) { // xor rax, rax: written, not read
                if (o == 0 && !destination.empty() && (writeOnly.contains(m) || m.starts_with("set"))) continue; // written, not read
                for (const std::string& reg: registers_in_(ops[o]))
                    if (!written.contains(reg) && reg != "rsp" && reg != "rbp") inputs.insert(reg);
            }
            if ((m == "mul" || m == "div" || m == "idiv" || m == "cdq" || m == "cqo" || m == "cdqe" || (m == "imul" && ops.size() == 1)) && !written.contains("rax")) inputs.insert("rax");
            if (m == "push" && ops.size() == 1 && !family_(ops[0]).empty()) {
                pushed.insert(family_(ops[0]));
                if (family_(ops[0]) == "rbp" && framePush == NO_INDEX) framePush = i;
                else stack = true;
            }
            if ((m == "pop" && (ops.empty() || family_(ops[0]) != "rbp")) || m == "call" || m == "enter") stack = true;
            if ((m == "sub" || m == "add" || m == "and") && destination == "rsp") stack = true;
            for (const std::string& op: ops)
                for (const std::string& reg: registers_in_(op))
                    if (op.find('[') != std::string::npos && (reg == "rsp" || reg == "rbp")) stack = true; // [rbp + 8], [esp]

            // What is written
            std::vector<std::string> outputs;
            if (!destination.empty() && !readOnly.contains(m) && !jump_(m)) outputs.push_back(destination);
            if (m == "xchg" && ops.size() == 2 && !family_(ops[1]).empty()) outputs.push_back(family_(ops[1]));
            if (m == "mul" || m == "div" || m == "idiv" || m == "cqo" || m == "cdq" || (m == "imul" && ops.size() == 1)) outputs.insert(outputs.end(), {"rax", "rdx"});
            if (m == "call" || m == "syscall") outputs.insert(outputs.end(), {"rax", "rcx", "r11"});
            for (const std::string& reg: outputs) {
                written.insert(reg);
                if (m != "pop" && !firstWrite.contains(reg)) firstWrite[reg] = i;
            }
        }

        // ABI: the callee-saved registers of a global function
        if (function.global)
            for (const auto &[reg, line]: firstWrite)
                if (calleeSaved.contains(reg) && !pushed.contains(reg))
                    issues.push_back(xstyle::check::make_issue(file, line, lines[line].mnemonicColumn, "ASM-CALLEE-SAVED", reg + " changed by " + function.name + " without push / pop",
                        "push " + reg + " at the start, pop " + reg + " before each ret (System V: rbx, rbp, r12-r15 are kept by the callee)"));

        // Stack frame: push rbp ... pop rbp / leave before each ret
        bool restored = false;
        for (const std::size_t i: body) {
            const std::string& m = lines[i].mnemonic;
            if (m == "leave" || (m == "pop" && !lines[i].operands.empty() && family_(lines[i].operands[0]) == "rbp")) restored = true;
        }
        if (framePush == NO_INDEX && restored) {
            for (const std::size_t i: body)
                if (lines[i].mnemonic == "leave" || lines[i].mnemonic == "pop") {
                    if (lines[i].mnemonic == "pop" && family_(lines[i].operands.empty() ? "" : lines[i].operands[0]) != "rbp") continue;
                    issues.push_back(xstyle::check::make_issue(file, i, lines[i].mnemonicColumn, "ASM-FRAME", lines[i].mnemonic + " of rbp without its push rbp in " + function.name,
                        "push rbp / mov rbp, rsp at the start, or remove it"));
                    break;
                }
        }
        if (framePush != NO_INDEX) {
            for (std::size_t b = 0; b < body.size(); ++b) {
                if (!lines[body[b]].mnemonic.starts_with("ret")) continue;
                bool found = false;
                for (std::size_t k = b; k-- > 0;) {
                    const AsmLine& previous = lines[body[k]];
                    if (body[k] == framePush || terminal_(previous.mnemonic)) break;
                    if (previous.mnemonic == "leave" || (previous.mnemonic == "pop" && !previous.operands.empty() && family_(previous.operands[0]) == "rbp")) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    issues.push_back(xstyle::check::make_issue(file, body[b], lines[body[b]].mnemonicColumn, "ASM-FRAME", "ret without pop rbp / leave (rbp pushed line " + std::to_string(framePush + 1) + ")",
                        "pop rbp (or leave) before the ret"));
            }
            if (!stack)
                issues.push_back(xstyle::check::make_issue(file, framePush, lines[framePush].mnemonicColumn, "ASM-FRAME-UNUSED", "Stack frame of " + function.name + " not needed (no other push / pop, no call, rsp and [rbp] unused)",
                    "remove push rbp / mov rbp, rsp and its pop rbp"));
        }

        // Fall through: the last instruction never continues into the next function
        const AsmLine& last = lines[body.back()];
        bool exits = terminal_(last.mnemonic) || (last.mnemonic == "call" && !last.operands.empty() && std::regex_search(last.operands[0], std::regex(R"(^(_?exit|abort)\b)")));
        const bool legacy = last.mnemonic == "int" && !last.operands.empty() && xstyle::trim(last.operands[0]) == "0x80"; // 32-bit: exit = 1
        if (last.mnemonic == "syscall" || legacy)
            for (std::size_t k = body.size() - 1; k-- > 0 && k + 5 > body.size() && !exits;) {
                long number = -1;
                const AsmLine& previous = lines[body[k]];
                exits = previous.mnemonic == "mov" && previous.operands.size() == 2 && family_(previous.operands[0]) == "rax" && number_(previous.operands[1], number)
                    && (legacy ? number == 1 : number == 60 || number == 231);
            }
        if (!exits)
            issues.push_back(xstyle::check::make_issue(file, body.back(), last.mnemonicColumn, "ASM-FALLTHROUGH",
                f + 1 < functions.size() && functions[f + 1].start == function.end ? "Function " + function.name + " falls through into " + functions[f + 1].name : "Function " + function.name + " ends without ret / jmp",
                "end it with ret (or jmp / the exit syscall)"));

        // Its registers on the line of its label: ; in: rdi, rsi | out: rax
        if (lines[function.start].comment) continue;
        bool output = false;
        for (std::size_t b = 0; b < body.size() && !output; ++b) {
            if (!lines[body[b]].mnemonic.starts_with("ret")) continue;
            for (std::size_t k = b; k-- > 0;) {
                const AsmLine& previous = lines[body[k]];
                if (terminal_(previous.mnemonic) || jump_(previous.mnemonic)) break;
                if (previous.mnemonic == "call" || previous.mnemonic == "syscall" || (!previous.operands.empty() && family_(previous.operands[0]) == "rax"
                    && !readOnly.contains(previous.mnemonic))) {
                    output = true;
                    break;
                }
            }
        }
        std::string in;
        for (const std::string& reg: {"rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "rbx", "r10", "r11", "r12", "r13", "r14", "r15"})
            if (inputs.contains(reg)) in += (in.empty() ? "" : ", ") + std::string(reg);
        const std::string comment = "; in: " + (in.empty() ? std::string("-") : in) + " | out: " + (output ? "rax" : "-");
        xstyle::Fix fix = xstyle::check::replace_line(function.start, text[function.start].substr(0, lines[function.start].codeEnd) + " " + comment);
        fix.unsafe = true; // registers guessed from the code: check them
        issues.push_back(xstyle::check::make_issue(file, function.start, lines[function.start].labelColumn, "ASM-REG-COMMENT", "Function " + function.name + " without the comment of its registers", "", fix));
    }
}

_hot static void instructions_(const xstyle::SourceFile& file, const std::vector<AsmLine>& lines, xstyle::check::Issues& issues)
{
    const std::vector<std::string>& text = file.getLines();

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const AsmLine& line = lines[i];
        long number = -1;

        // mov reg, 0 -> xor reg, reg (shorter, but the flags change: --force)
        if (line.mnemonic == "mov" && line.operands.size() == 2 && !family_(line.operands[0]).empty() && number_(line.operands[1], number) && number == 0) {
            const std::string reg = line.operands[0];
            const std::size_t end = text[i].find(line.operands[1], line.mnemonicColumn);
            xstyle::Fix fix = xstyle::check::replace_line(i, text[i].substr(0, line.mnemonicColumn) + "xor " + reg + ", " + reg + text[i].substr(end == std::string::npos ? text[i].size() : end + line.operands[1].size()));
            fix.unsafe = true;
            issues.push_back(xstyle::check::make_issue(file, i, line.mnemonicColumn, "ASM-ZERO-XOR", "mov " + reg + ", 0", "", fix));
        }

        // syscall: the mov of its number named in a comment
        if (line.mnemonic != "syscall") continue;
        for (std::size_t k = i; k-- > 0 && k + 9 > i;) {
            const AsmLine& previous = lines[k];
            if (!previous.label.empty() || jump_(previous.mnemonic) || terminal_(previous.mnemonic) || previous.mnemonic == "syscall") break;
            if (previous.mnemonic != "mov" || previous.operands.size() != 2 || family_(previous.operands[0]) != "rax") continue;
            if (previous.comment || !number_(previous.operands[1], number)) break;
            const std::string name = syscall_name_(number);
            if (name.empty())
                issues.push_back(xstyle::check::make_issue(file, k, previous.mnemonicColumn, "ASM-SYSCALL-COMMENT", "Syscall " + std::to_string(number) + " without the comment of its name", "mov rax, " + previous.operands[1] + " ; name"));
            else
                issues.push_back(xstyle::check::make_issue(file, k, previous.mnemonicColumn, "ASM-SYSCALL-COMMENT", "Syscall " + std::to_string(number) + " (" + name + ") without the comment of its name", "",
                    xstyle::check::replace_line(k, text[k].substr(0, previous.codeEnd) + " ; " + name)));
            break;
        }
    }
}

/* checks */
_hot void xstyle::check::assembly(const xstyle::SourceFile& file, xstyle::check::Issues& issues)
{
    header_(file, issues);
    if (file.isGas()) return; // GAS syntax: the header and the generic rules only
    const std::vector<AsmLine> lines = parse_(file);
    sections_(file, lines, issues);
    indentation_(file, lines, issues);
    comments_(file, lines, issues);
    case_(file, lines, issues);
    const std::vector<AsmFunction> functions = names_(file, lines, issues);
    functions_(file, lines, functions, issues);
    instructions_(file, lines, issues);
}
