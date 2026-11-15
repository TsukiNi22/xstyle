/**************************************************************\
Edition:
##  @date 06/10/2026 by @author Tsukini

File Name:
##  @file Checker.hpp

File Description:
##  Runs the checks on a file (selection, suppression comments) and applies the automatic fixes
\**************************************************************/

#ifndef CHECKER_H
    #define CHECKER_H

    //----------------------------------------------------------------//
    /* INCLUDE */

    /* type */
    #define _Attribute
    #include <utils/utils.hpp>      // _hot, _cold, _nodiscard
    #include "XStyleType.hpp"       // xstyle::Options, xstyle::ProjectInfo, xstyle::Issue, xstyle::AppliedFix
    #include "SourceFile.hpp"       // xstyle::SourceFile
    #include "Ignore.hpp"           // xstyle::Ignore
    #include <string_view>          // std::string_view
    #include <vector>               // std::vector

    //----------------------------------------------------------------//
    /* DEFINE */

    #define FIX_PASSES 32 // Maximum passes of the fix loop (a fix can reveal another issue)

namespace xstyle { // namespace start
//----------------------------------------------------------------//
/* CLASS */

class Checker {
    private:
        const xstyle::Options& _options;
        const xstyle::ProjectInfo& _project;
        const xstyle::Ignore* _ignore = nullptr; // rules ignored by path (nullptr: none)
        bool _libutils = false; // libutils rules enabled

        // ---------- Pre-Function -------- //
        _hot _nodiscard bool chosen_(std::string_view code, const bool fix) const; // -c / --fix CODES
        _hot _nodiscard bool selected_(std::string_view code, const bool fix) const; // chosen and not ignored (-i)
        _hot _nodiscard bool suppressed_(const std::vector<xstyle::Directive>& directives, const xstyle::Issue& issue) const;
        _hot void unused_(const xstyle::SourceFile& file, const std::vector<xstyle::Directive>& directives, std::vector<xstyle::Issue>& issues) const; // G-UNUSED-IGNORE
        _hot _nodiscard bool changed_(const xstyle::SourceFile& file, const std::size_t line) const;
        _hot _nodiscard std::vector<xstyle::Issue> run_(const xstyle::SourceFile& file, const bool fix, std::vector<xstyle::Issue>* ignored = nullptr) const;

    public:
        // ---------- Pre-Function -------- //
        _hot std::vector<xstyle::AppliedFix> fix(xstyle::SourceFile& file) const; // fix loop, returns what was changed
        _hot _nodiscard bool skipped(const xstyle::SourceFile& file) const; // xstyle: skip-file (not checked nor counted)
        _hot _nodiscard static std::vector<xstyle::Directive> directives(const xstyle::SourceFile& file); // xstyle: ... comments of the file

        // ------------ Function ---------- //
        _hot _nodiscard inline std::vector<xstyle::Issue> check(const xstyle::SourceFile& file) const                                        {return this->run_(file, false);};
        _hot _nodiscard inline std::vector<xstyle::Issue> check(const xstyle::SourceFile& file, std::vector<xstyle::Issue>& ignored) const {return this->run_(file, false, &ignored);}; // + the ignored ones
        _cold _nodiscard inline bool libutils(void) const                                             {return this->_libutils;};
        _cold _nodiscard inline bool wants(std::string_view code) const                               {return this->selected_(code, true);}; // code selected for the fixes
        _cold _nodiscard inline bool shows(std::string_view code) const                               {return this->selected_(code, false);}; // code selected for the display

        // ------------ Operator ---------- //
        Checker& operator=(const Checker& other) = delete;
        Checker& operator=(Checker&& other) = delete;

        // ---------- Constructor --------- //
        Checker(const xstyle::Options& options, const xstyle::ProjectInfo& project, const xstyle::Ignore* ignore = nullptr);
        Checker(const Checker& other) = delete;
        Checker(Checker&& other) = delete;

        // ----------- Destructor --------- //
        ~Checker() = default;
};

} // namespace end
#endif /* CHECKER_H */
