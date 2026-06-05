#include <iostream>
#include <string>
#include <cassert>
#include "../single_include/nlohmann/json.hpp"
using json = nlohmann::json;
std::string make_deep_json(int depth) {
    std::string s(depth, '[');
    s += "42";
    s += std::string(depth, ']');
    return s;
}
std::string make_deep_object_json(int depth) {
    std::string s;
    for (int i = 0; i < depth; ++i) { s += "{\"k\":"; }
    s += "42";
    for (int i = 0; i < depth; ++i) { s += "}"; }
    return s;
}
int main() {
    std::cout << "=== Max Depth Test Suite ===" << std::endl;
    try {
        auto j1 = json::parse(R"({"hello":"world","arr":[1,2,3]})");
        assert(j1["hello"] == "world");
        assert(j1["arr"][1] == 2);
        std::cout << "Test 1 PASSED: backward compatible" << std::endl;
    } catch (...) { std::cerr << "Test 1 FAILED" << std::endl; return 1; }
    try {
        auto s = make_deep_json(50);
        auto j = json::parse(s, nullptr, true, false, false, 100);
        json* cur = &j;
        for (int i = 0; i < 50; ++i) cur = &(*cur)[0];
        assert(*cur == 42);
        std::cout << "Test 2 PASSED: within limit" << std::endl;
    } catch (...) { std::cerr << "Test 2 FAILED" << std::endl; return 1; }
    try {
        auto s = make_deep_json(200);
        auto j = json::parse(s, nullptr, true, false, false, 100);
        std::cerr << "Test 3 FAILED: no exception" << std::endl; return 1;
    } catch (const json::parse_error& e) {
        std::cout << "Test 3 PASSED: array depth exceeded: " << e.what() << std::endl;
    }
    try {
        auto s = make_deep_object_json(200);
        auto j = json::parse(s, nullptr, true, false, false, 100);
        std::cerr << "Test 4 FAILED: no exception" << std::endl; return 1;
    } catch (const json::parse_error& e) {
        std::cout << "Test 4 PASSED: object depth exceeded: " << e.what() << std::endl;
    }
    auto s5 = make_deep_json(200);
    bool r5 = json::accept(s5, false, false, 100);
    assert(!r5);
    std::cout << "Test 5 PASSED: accept rejects deep" << std::endl;
    auto s6 = make_deep_json(50);
    bool r6 = json::accept(s6, false, false, 100);
    assert(r6);
    std::cout << "Test 6 PASSED: accept allows shallow" << std::endl;
    try {
        auto j = json::parse(R"([1])", nullptr, true, false, false, 0);
        std::cerr << "Test 7 FAILED: no exception" << std::endl; return 1;
    } catch (const json::parse_error& e) {
        std::cout << "Test 7 PASSED: depth=0 rejects nesting" << std::endl;
    }
    auto j8 = json::parse("42", nullptr, true, false, false, 0);
    assert(j8 == 42);
    std::cout << "Test 8 PASSED: scalar with depth=0" << std::endl;
    std::cout << std::endl << "=== ALL TESTS PASSED ===" << std::endl;
    return 0;
}
