//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++ (supporting code)
// |  |  |__   |  |  | | | |  version 3.12.0
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2026 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#include "doctest_compatibility.h"

#define JSON_TESTS_PRIVATE
#include <nlohmann/json.hpp>
using nlohmann::json;

#include <limits>
#include <string>
#include <sstream>

namespace
{
// Helper: generate a deeply nested JSON array: [[[[...42...]]]]
std::string make_deep_array(int depth)
{
    std::string s(static_cast<std::size_t>(depth), '[');
    s += "42";
    s += std::string(static_cast<std::size_t>(depth), ']');
    return s;
}

// Helper: generate a deeply nested JSON object: {"k":{"k":{..."k":42...}}}
std::string make_deep_object(int depth)
{
    std::string s;
    for (int i = 0; i < depth; ++i)
    {
        s += "{\"k\":";
    }
    s += "42";
    for (int i = 0; i < depth; ++i)
    {
        s += "}";
    }
    return s;
}

// Helper: generate mixed array/object nesting
std::string make_deep_mixed(int depth)
{
    std::string s;
    for (int i = 0; i < depth; ++i)
    {
        s += (i % 2 == 0) ? "[" : "{\"x\":";
    }
    s += "42";
    for (int i = 0; i < depth; ++i)
    {
        s += (i % 2 == 0) ? "]" : "}";
    }
    return s;
}
} // namespace

TEST_CASE("max_depth - basic functionality")
{
    SECTION("parse with max_depth parameter")
    {
        // depth 5, limit 10 -> should succeed
        CHECK_NOTHROW(json::parse(make_deep_array(5), nullptr, true, false, false, 10));
        CHECK_NOTHROW(json::parse(make_deep_object(5), nullptr, true, false, false, 10));

        // depth 20, limit 10 -> should fail with parse_error.101
        CHECK_THROWS_WITH_AS(
            json::parse(make_deep_array(20), nullptr, true, false, false, 10),
            "[json.exception.parse_error.101] parse error at line 1, column 11: "
            "maximum nesting depth of 10 exceeded",
            json::parse_error&);
        CHECK_THROWS_WITH_AS(
            json::parse(make_deep_object(20), nullptr, true, false, false, 10),
            "[json.exception.parse_error.101] parse error at line 1, column 51: "
            "maximum nesting depth of 10 exceeded",
            json::parse_error&);
    }

    SECTION("parse without max_depth uses default (unlimited)")
    {
        // default behavior: no limit, should parse normally
        auto j1 = json::parse(make_deep_array(100));
        CHECK(j1.is_array());
        auto j2 = json::parse(make_deep_object(100));
        CHECK(j2.is_object());
    }

    SECTION("accept with max_depth")
    {
        CHECK(json::accept(make_deep_array(5), false, false, 10));
        CHECK(json::accept(make_deep_object(5), false, false, 10));

        CHECK_FALSE(json::accept(make_deep_array(20), false, false, 10));
        CHECK_FALSE(json::accept(make_deep_object(20), false, false, 10));
    }

    SECTION("accept without max_depth uses default (unlimited)")
    {
        CHECK(json::accept(make_deep_array(100)));
        CHECK(json::accept(make_deep_object(100)));
    }
}

TEST_CASE("max_depth - edge cases")
{
    SECTION("max_depth = 0 rejects any nesting")
    {
        // scalar values have depth 0 (no nesting), should be ok
        CHECK_NOTHROW(json::parse("42", nullptr, true, false, false, 0));
        CHECK_NOTHROW(json::parse("true", nullptr, true, false, false, 0));
        CHECK_NOTHROW(json::parse("\"hello\"", nullptr, true, false, false, 0));
        CHECK_NOTHROW(json::parse("null", nullptr, true, false, false, 0));

        // empty array/object have depth 1
        CHECK_THROWS_AS(json::parse("[]", nullptr, true, false, false, 0), json::parse_error&);
        CHECK_THROWS_AS(json::parse("{}", nullptr, true, false, false, 0), json::parse_error&);

        // nested array has depth 2
        CHECK_THROWS_AS(json::parse("[1]", nullptr, true, false, false, 0), json::parse_error&);
    }

    SECTION("max_depth = 1 allows single-level containers")
    {
        CHECK_NOTHROW(json::parse("[]", nullptr, true, false, false, 1));
        CHECK_NOTHROW(json::parse("{}", nullptr, true, false, false, 1));
        CHECK_NOTHROW(json::parse("[1,2,3]", nullptr, true, false, false, 1));
        CHECK_NOTHROW(json::parse("{\"a\":1}", nullptr, true, false, false, 1));

        // nested is depth 2 -> should fail
        CHECK_THROWS_AS(json::parse("[[]]", nullptr, true, false, false, 1), json::parse_error&);
        CHECK_THROWS_AS(json::parse("{\"a\":{}}", nullptr, true, false, false, 1), json::parse_error&);
    }
}

TEST_CASE("max_depth - exact boundary")
{
    SECTION("exactly at limit -> success")
    {
        CHECK_NOTHROW(json::parse(make_deep_array(50), nullptr, true, false, false, 50));
        CHECK_NOTHROW(json::parse(make_deep_object(50), nullptr, true, false, false, 50));
    }

    SECTION("one above limit -> failure")
    {
        CHECK_THROWS_AS(json::parse(make_deep_array(51), nullptr, true, false, false, 50), json::parse_error&);
        CHECK_THROWS_AS(json::parse(make_deep_object(51), nullptr, true, false, false, 50), json::parse_error&);
    }
}

TEST_CASE("max_depth - mixed nesting")
{
    SECTION("mixed array/object counts toward same limit")
    {
        CHECK_NOTHROW(json::parse(make_deep_mixed(5), nullptr, true, false, false, 10));
        CHECK_THROWS_AS(json::parse(make_deep_mixed(15), nullptr, true, false, false, 10), json::parse_error&);
    }
}

TEST_CASE("max_depth - error code consistent")
{
    SECTION("error id is 101 (same as other parse errors)")
    {
        try
        {
            json::parse(make_deep_array(100), nullptr, true, false, false, 10);
            CHECK(false); // should not reach here
        }
        catch (const json::parse_error& e)
        {
            CHECK(e.id == 101);
        }
    }

    SECTION("exception type is parse_error")
    {
        CHECK_THROWS_AS(
            json::parse(make_deep_array(100), nullptr, true, false, false, 10),
            json::parse_error);
    }
}

TEST_CASE("max_depth - various input types")
{
    SECTION("string input")
    {
        CHECK_NOTHROW(json::parse(make_deep_array(5), nullptr, true, false, false, 10));
        CHECK_THROWS_AS(json::parse(make_deep_array(100), nullptr, true, false, false, 10), json::parse_error&);
    }

    SECTION("C-string input")
    {
        CHECK_NOTHROW(json::parse(std::string(make_deep_array(5)).c_str(), nullptr, true, false, false, 10));
        CHECK_THROWS_AS(
            json::parse(std::string(make_deep_array(100)).c_str(), nullptr, true, false, false, 10),
            json::parse_error&);
    }

    SECTION("iterator pair input")
    {
        auto s_ok = make_deep_array(5);
        auto s_bad = make_deep_array(100);

        CHECK_NOTHROW(json::parse(s_ok.begin(), s_ok.end(), nullptr, true, false, false, 10));
        CHECK_THROWS_AS(
            json::parse(s_bad.begin(), s_bad.end(), nullptr, true, false, false, 10),
            json::parse_error&);
    }

    SECTION("stream input")
    {
        std::istringstream ss_ok(make_deep_array(5));
        std::istringstream ss_bad(make_deep_array(100));

        CHECK_NOTHROW(json::parse(ss_ok, nullptr, true, false, false, 10));
        CHECK_THROWS_AS(
            json::parse(ss_bad, nullptr, true, false, false, 10),
            json::parse_error&);
    }
}

TEST_CASE("max_depth - large depth with small limit (DoS prevention)")
{
    SECTION("rejects extreme depth instantly")
    {
        // Simulating a DoS attack: very deep nesting with small limit
        // The test should pass very quickly (no O(N) processing needed
        // if depth is checked early in parser begin_array/begin_object)
        auto attack_payload = make_deep_array(10000);
        CHECK_THROWS_AS(
            json::parse(attack_payload, nullptr, true, false, false, 256),
            json::parse_error&);
    }
}

TEST_CASE("max_depth - allow_exceptions=false")
{
    SECTION("returns discarded value on depth exceeded")
    {
        // With allow_exceptions=false, parse returns a discarded value on error
        auto j = json::parse(make_deep_array(100), nullptr, false, false, false, 10);
        CHECK(j.is_discarded());
    }
}

TEST_CASE("max_depth - ignore_comments interaction")
{
    SECTION("depth check works with comment parsing enabled")
    {
        // Comments don't add depth, they just add tokens
        // Depth of this JSON is 3 (outer array, inner array, inner-inner array)
        std::string json_with_comments =
            "[ // level 0\n"
            "  [ // level 1\n"
            "    /* level 2 */ 42\n"
            "  ]\n"
            "]";

        // depth 3, limit 5 -> ok
        CHECK_NOTHROW(json::parse(json_with_comments, nullptr, true, true, false, 5));

        // depth 2, limit 1 -> fail (limit==1 is too tight)
        CHECK_THROWS_AS(
            json::parse(json_with_comments, nullptr, true, true, false, 1),
            json::parse_error&);
    }
}

TEST_CASE("max_depth - multiple parses in sequence")
{
    SECTION("limit does not leak state between parses")
    {
        // Parse at depth with limit, then parse below limit
        CHECK_THROWS_AS(
            json::parse(make_deep_array(100), nullptr, true, false, false, 10),
            json::parse_error&);

        // Subsequent parse should work fine
        CHECK_NOTHROW(json::parse("{\"hello\":\"world\"}", nullptr, true, false, false, 10));
    }
}

TEST_CASE("max_depth - comparison with rapidjson default")
{
    SECTION("limit=256 matches rapidjson kDefaultMaxDepth")
    {
        // rapidjson uses 256 as default max depth
        CHECK_NOTHROW(json::parse(make_deep_array(200), nullptr, true, false, false, 256));
        CHECK_THROWS_AS(
            json::parse(make_deep_array(300), nullptr, true, false, false, 256),
            json::parse_error&);
    }
}
