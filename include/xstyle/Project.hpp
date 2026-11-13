/**************************************************************\
Edition:
##  @date 06/10/2026 by @author Tsukini

File Name:
##  @file Project.hpp

File Description:
##  Detection of the project: root, libutils installed / used, migration aliases, attribute macros
\**************************************************************/

#ifndef PROJECT_H
    #define PROJECT_H

    //----------------------------------------------------------------//
    /* INCLUDE */

    /* type */
    #define _Attribute
    #include <utils/utils.hpp>      // _cold, _nodiscard
    #include "XStyleType.hpp"       // xstyle::ProjectInfo, xstyle::Language
    #include <filesystem>           // std::filesystem::path
    #include <string>               // std::string
    #include <vector>               // std::vector

    //----------------------------------------------------------------//
    /* DEFINE */

    #define MAX_PROJECT_FILES 20000 // Entries walked in the project to read its sources
    #define MAX_SCANNED_SOURCES 3000 // C / C++ files read for their function declarations
    #define MAX_SCANNED_SIZE 1048576 // Bytes of a C / C++ file read for its function declarations

namespace xstyle { // namespace start
//----------------------------------------------------------------//
/* PROTOTYPE */

/* project */
_cold _nodiscard xstyle::ProjectInfo detect_project(const std::filesystem::path& start);
_cold _nodiscard xstyle::Language language_of(const std::filesystem::path& path);
_cold _nodiscard bool skipped_directory(const std::string& name); // build*, hidden, vendored... never scanned
_cold void scan_sources(xstyle::ProjectInfo& project); // C / C++ sources of the whole project (from its root)

} // namespace end
#endif /* PROJECT_H */
