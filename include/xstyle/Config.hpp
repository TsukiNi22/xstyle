/**************************************************************\
Edition:
##  @date 15/11/2026 by @author Tsukini

File Name:
##  @file Config.hpp

File Description:
##  .xconfig of a project (TOML subset): default options and [[path]] tables of rules ignored / paths excluded
\**************************************************************/

#ifndef CONFIG_H
    #define CONFIG_H

    //----------------------------------------------------------------//
    /* INCLUDE */

    /* type */
    #define _Attribute
    #include <utils/utils.hpp>      // _cold, _nodiscard
    #include "Ignore.hpp"           // xstyle::PathRule
    #include <filesystem>           // std::filesystem::path
    #include <optional>             // std::optional
    #include <string>               // std::string
    #include <vector>               // std::vector

    //----------------------------------------------------------------//
    /* DEFINE */

    #define XCONFIG_FILE ".xconfig" // Project configuration, the closest one from the working directory up to the project root

namespace xstyle { // namespace start
//----------------------------------------------------------------//
/* STRUCT */

struct ConfigOption {
    std::string key; // name of the key (severity, ignore, gitignore...)
    std::vector<std::string> values; // a string / integer / boolean: one value, an array: its items
    std::string source; // <file>:<line> (error messages)
};

struct Config {
    std::filesystem::path file; // the .xconfig read
    std::vector<xstyle::ConfigOption> options; // top-level keys, in order
    std::vector<xstyle::PathRule> paths; // [[path]] tables (match + ignore / exclude)
};

//----------------------------------------------------------------//
/* PROTOTYPE */

/* config */
_cold _nodiscard std::optional<std::filesystem::path> find_config(const std::filesystem::path& start, const std::filesystem::path& root); // start, its parents up to root
_cold _nodiscard xstyle::Config read_config(const std::filesystem::path& file); // throws on a syntax error or an unknown key (<file>:<line>)

} // namespace end
#endif /* CONFIG_H */
