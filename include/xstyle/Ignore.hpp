/**************************************************************\
Edition:
##  @date 15/11/2026 by @author Tsukini

File Name:
##  @file Ignore.hpp

File Description:
##  Paths and rules ignored by path: .xignore files, [[path]] of .xconfig, --ignore-path and the files ignored by git
\**************************************************************/

#ifndef IGNORE_H
    #define IGNORE_H

    //----------------------------------------------------------------//
    /* INCLUDE */

    /* type */
    #define _Attribute
    #include <utils/utils.hpp>      // _hot, _cold, _nodiscard
    #include <unordered_set>        // std::unordered_set
    #include <filesystem>           // std::filesystem::path
    #include <optional>             // std::optional
    #include <string>               // std::string
    #include <vector>               // std::vector
    #include <map>                  // std::map

    //----------------------------------------------------------------//
    /* DEFINE */

    #define XIGNORE_FILE ".xignore" // Paths (and rules by path) ignored, one file per directory like a .gitignore

namespace xstyle { // namespace start
//----------------------------------------------------------------//
/* STRUCT */

struct PathRule {
    std::filesystem::path base; // directory the pattern is relative to (absolute)
    std::vector<std::string> segments; // pattern split on '/' (** = any number of directories)
    bool negate = false; // !pattern: included again (or these codes no longer ignored)
    bool directoryOnly = false; // pattern/: directories only
    std::vector<std::string> codes; // empty: the paths are not checked, else these rules are ignored there
    std::string source; // <file>:<line> or the flag (error messages)
};

//----------------------------------------------------------------//
/* CLASS */

class Ignore {
    private:
        std::filesystem::path _root; // project root: the .xignore files are read from it down to the file
        bool _xignore = true; // the .xignore files are read (--no-inline-ignore: no)
        bool _git = false; // the files ignored by git are skipped
        std::vector<xstyle::PathRule> _first; // [[path]] of .xconfig (before the .xignore files)
        std::vector<xstyle::PathRule> _last; // --ignore-path (after the .xignore files)
        std::unordered_set<std::string> _gitIgnored; // absolute paths ignored by git (directories without their final /)
        mutable std::map<std::filesystem::path, std::vector<xstyle::PathRule>> _cache; // directory -> rules of its .xignore

        // ---------- Pre-Function -------- //
        _cold _nodiscard const std::vector<xstyle::PathRule>& directory_(const std::filesystem::path& directory) const;
        _cold _nodiscard std::vector<const xstyle::PathRule*> rules_(const std::filesystem::path& path) const; // in their order of precedence
        _hot _nodiscard static bool matches_(const xstyle::PathRule& rule, const std::filesystem::path& path, const bool directory);

    public:
        // ---------- Pre-Function -------- //
        /* setup */
        _cold void loadGit(const std::filesystem::path& repository); // git ls-files --others --ignored --exclude-standard --directory

        /* query */
        _hot _nodiscard bool excluded(const std::filesystem::path& path, const bool directory) const; // .xignore / [[path]] (the directory or one of its parents)
        _hot _nodiscard bool gitIgnored(const std::filesystem::path& path) const;
        _hot _nodiscard std::vector<std::string> codes(const std::filesystem::path& file) const; // rules ignored for this file

        /* parse */
        _cold _nodiscard static std::optional<xstyle::PathRule> parse(const std::string& line, const std::filesystem::path& base, const std::string& source); // throws on bad codes

        // ------------ Function ---------- //
        _cold inline void setGit(const bool git)                                 {this->_git = git;};
        _cold inline void addRule(const xstyle::PathRule& rule, const bool last) {(last ? this->_last : this->_first).push_back(rule);}; // last: --ignore-path, else .xconfig

        // ------------ Operator ---------- //
        Ignore& operator=(const Ignore& other) = delete;
        Ignore& operator=(Ignore&& other) = delete;

        // ---------- Constructor --------- //
        Ignore(const std::filesystem::path& root, const bool xignore);
        Ignore(const Ignore& other) = delete;
        Ignore(Ignore&& other) = delete;

        // ----------- Destructor --------- //
        ~Ignore() = default;
};

} // namespace end
#endif /* IGNORE_H */
