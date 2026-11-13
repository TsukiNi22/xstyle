/**************************************************************\
Edition:
##  @date 09/11/2026 by @author Tsukini

File Name:
##  @file Rules.cpp

File Description:
##  Unit tests of the rule catalogue (lookup, patterns, names)
\**************************************************************/

#include "xstyle/Rules.hpp"
#include <unordered_set>
#include <gtest/gtest.h>
#include <string>

/* Catalogue */
TEST(Rules, CodesAreUnique)
{
    std::unordered_set<std::string_view> codes;

    for (const xstyle::Rule& rule: xstyle::rules())
        EXPECT_TRUE(codes.insert(rule.code).second) << "duplicated code " << rule.code;
    EXPECT_FALSE(codes.empty());
}

TEST(Rules, EveryRuleIsDescribed)
{
    for (const xstyle::Rule& rule: xstyle::rules()) {
        EXPECT_FALSE(rule.description.empty()) << rule.code;
        EXPECT_FALSE(rule.languages.empty()) << rule.code;
    }
}

TEST(Rules, FindKnownRule)
{
    const xstyle::Rule* rule = xstyle::find_rule("G-TAB");

    ASSERT_NE(rule, nullptr);
    EXPECT_EQ(rule->severity, xstyle::Severity::Major);
    EXPECT_EQ(rule->fix, xstyle::FixMode::Auto);
}

TEST(Rules, FindUnknownRule)
{
    EXPECT_EQ(xstyle::find_rule("NOT-A-RULE"), nullptr);
    EXPECT_EQ(xstyle::find_rule(""), nullptr);
}

TEST(Rules, RuleAppliesToItsLanguages)
{
    const xstyle::Rule* generic = xstyle::find_rule("G-TAB");
    const xstyle::Rule* cpp = xstyle::find_rule("CPP-USING-NAMESPACE");

    ASSERT_NE(generic, nullptr);
    ASSERT_NE(cpp, nullptr);
    EXPECT_TRUE(xstyle::rule_applies(*generic, xstyle::Language::Python));
    EXPECT_TRUE(xstyle::rule_applies(*cpp, xstyle::Language::Cpp));
    EXPECT_FALSE(xstyle::rule_applies(*cpp, xstyle::Language::Python));
}

TEST(Rules, CAndCppCodesAreSeparate)
{
    // A C file never gets a CPP- code: the rules shared with the C++ have their own C- twin
    for (const xstyle::Rule& rule: xstyle::rules()) {
        if (rule.code.starts_with("CPP-")) EXPECT_FALSE(xstyle::rule_applies(rule, xstyle::Language::C)) << rule.code;
        if (rule.code.starts_with("C-")) EXPECT_FALSE(xstyle::rule_applies(rule, xstyle::Language::Cpp)) << rule.code;
        if (rule.code.starts_with("C-")) EXPECT_TRUE(xstyle::rule_applies(rule, xstyle::Language::C)) << rule.code;
    }
    EXPECT_NE(xstyle::find_rule("C-GUARD-MISSING"), nullptr);
    EXPECT_FALSE(xstyle::match_code("CPP-PTR-REF", "C")); // -c C selects the C rules only
    EXPECT_TRUE(xstyle::match_code("C-PTR-REF", "C"));
}

/* Patterns */
TEST(Rules, MatchCode)
{
    EXPECT_TRUE(xstyle::match_code("G-TAB", "*"));
    EXPECT_TRUE(xstyle::match_code("G-TAB", "G-TAB"));
    EXPECT_TRUE(xstyle::match_code("CPP-USING-NAMESPACE", "CPP"));
    EXPECT_TRUE(xstyle::match_code("CPP-USING-NAMESPACE", "CPP-USING*"));
    EXPECT_FALSE(xstyle::match_code("CPPX-RULE", "CPP")); // a prefix only stops on a '-'
    EXPECT_FALSE(xstyle::match_code("G-TAB", "CPP"));
}

/* Names */
TEST(Rules, ParseSeverity)
{
    EXPECT_EQ(xstyle::parse_severity("major"), xstyle::Severity::Major);
    EXPECT_EQ(xstyle::parse_severity("MI"), xstyle::Severity::Minor);
    EXPECT_EQ(xstyle::parse_severity("u"), xstyle::Severity::Unforgivable);
    EXPECT_EQ(xstyle::parse_severity("n"), xstyle::Severity::Negligible);
    EXPECT_FALSE(xstyle::parse_severity("huge").has_value());
}

TEST(Rules, ParseLanguage)
{
    EXPECT_EQ(xstyle::parse_language("C++"), xstyle::Language::Cpp);
    EXPECT_EQ(xstyle::parse_language("python"), xstyle::Language::Python);
    EXPECT_EQ(xstyle::parse_language("bash"), xstyle::Language::Shell);
    EXPECT_EQ(xstyle::parse_language("rust"), xstyle::Language::Rust);
    EXPECT_FALSE(xstyle::parse_language("cobol").has_value());
}

TEST(Rules, LanguageIdRoundTrip)
{
    for (std::uint8_t i = 0; i <= static_cast<std::uint8_t>(xstyle::Language::Other); ++i) {
        const xstyle::Language language = static_cast<xstyle::Language>(i);
        EXPECT_EQ(xstyle::parse_language(std::string(xstyle::language_id(language))), language) << xstyle::language_name(language);
    }
}
