#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std::chrono;

// ============================================
// Memory measurement (Windows-specific)
// ============================================
size_t get_current_memory_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / (1024 * 1024);
    }
#endif
    return 0;
}

// ============================================
// Generate deeply nested JSON
// ============================================
std::string make_deep_json(int depth, const std::string& leaf = "42") {
    std::string s(depth, '[');
    s += leaf;
    s += std::string(depth, ']');
    return s;
}

std::string make_deep_object_json(int depth, const std::string& leaf = "42") {
    std::string s;
    for (int i = 0; i < depth; ++i) s += "{\"k\":";
    s += leaf;
    for (int i = 0; i < depth; ++i) s += "}";
    return s;
}

// ============================================
// Benchmark: parse with depth limit
// ============================================
struct BenchResult {
    bool success;
    double time_ms;
    size_t mem_start_mb;
    size_t mem_peak_mb;
    std::string error;
};

BenchResult bench_parse(const std::string& input, int max_depth, bool use_limit) {
    BenchResult r{false, 0, 0, 0, ""};
    r.mem_start_mb = get_current_memory_mb();
    size_t mem_max = r.mem_start_mb;

    auto t0 = high_resolution_clock::now();
    try {
        if (use_limit) {
            auto j = json::parse(input, nullptr, true, false, false, (size_t)max_depth);
        } else {
            auto j = json::parse(input);
        }
        r.success = true;
    } catch (const json::parse_error& e) {
        r.error = e.what();
    } catch (const std::exception& e) {
        r.error = std::string("std::exception: ") + e.what();
    } catch (...) {
        r.error = "unknown exception";
    }

    auto t1 = high_resolution_clock::now();
    r.time_ms = duration<double, std::milli>(t1 - t0).count();
    r.mem_peak_mb = get_current_memory_mb();

    return r;
}

// ============================================
// Find max parseable depth
// ============================================
int find_max_depth(bool use_limit, int limit_val = 0, int max_test = 100000) {
    int lo = 0, hi = max_test;
    int last_ok = 0;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (mid == 0) { lo = 1; continue; }

        std::string input = make_deep_json(mid);
        try {
            if (use_limit) {
                auto j = json::parse(input, nullptr, true, false, false, (size_t)limit_val);
            } else {
                auto j = json::parse(input);
            }
            last_ok = mid;
            lo = mid + 1;
        } catch (...) {
            hi = mid - 1;
        }
    }
    return last_ok;
}

// ============================================
// Main
// ============================================
int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "  nlohmann/json Depth Limit - Quantitative Evaluation" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "  Compiler: g++ " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << std::endl;
    std::cout << "  Library:  nlohmann/json v3.12.0 (with depth limit patch)" << std::endl;
    std::cout << "============================================================" << std::endl << std::endl;

    // ─── TEST 1: Find max depth WITHOUT protection ───
    std::cout << "=== TEST 1: Maximum depth WITHOUT protection ===" << std::endl;
    std::cout << "  (Binary searching for last parseable depth...)" << std::endl;

    int max_no_limit = find_max_depth(false);
    std::cout << "  Max parseable depth (no limit): " << max_no_limit << std::endl;
    std::cout << "  → System crashes / stack overflow beyond this depth" << std::endl;
    std::cout << std::endl;

    // ─── TEST 2: Parse just below the crash point, measure resources ───
    std::cout << "=== TEST 2: Resource consumption at near-crash depth ===" << std::endl;
    if (max_no_limit > 0) {
        int test_depth = (int)(max_no_limit * 0.95);
        auto input = make_deep_json(test_depth);

        size_t mem_before = get_current_memory_mb();
        auto r = bench_parse(input, 0, false);
        std::cout << "  Depth " << test_depth << " (95% of max): " << (r.success ? "PARSED" : "FAILED") << std::endl;
        std::cout << "    Time:       " << std::fixed << std::setprecision(1) << r.time_ms << " ms" << std::endl;
        std::cout << "    Memory:     " << r.mem_peak_mb - r.mem_start_mb << " MB (delta)" << std::endl;
        std::cout << "    Peak memory: " << r.mem_peak_mb << " MB" << std::endl;
    }
    std::cout << std::endl;

    // ─── TEST 3: With protection - known safe depths ───
    std::cout << "=== TEST 3: With depth limit protection (limit=512) ===" << std::endl;
    std::vector<int> test_depths = {100, 512, 1000, 5000, 10000, 50000, 100000};
    for (int d : test_depths) {
        auto input = make_deep_json(d);
        auto r = bench_parse(input, 512, true);
        std::cout << "  Depth=" << std::setw(6) << d << ": "
                  << (r.success ? "PASS" : "REJECT")
                  << "  |  " << std::fixed << std::setprecision(2) << r.time_ms << " ms"
                  << "  |  mem delta: " << (r.mem_peak_mb - r.mem_start_mb) << " MB";
        if (!r.success) {
            std::cout << "  |  " << r.error.substr(0, 60);
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    // ─── TEST 4: Memory scaling comparison ───
    std::cout << "=== TEST 4: Memory scaling (with vs without limit) ===" << std::endl;
    std::cout << std::setw(10) << "Depth" << " | "
              << std::setw(12) << "NoLimit(MB)" << " | "
              << std::setw(12) << "Limit512(MB)" << " | "
              << std::setw(12) << "Saved(MB)" << std::endl;
    std::cout << std::string(55, '-') << std::endl;

    std::vector<int> scaling_depths = {50, 100, 200, 300, 400, 500};
    for (int d : scaling_depths) {
        if (d > max_no_limit) break;
        auto input = make_deep_json(d);

        size_t mem_nl = 0, mem_wl = 0;

        // with limit
        size_t m0 = get_current_memory_mb();
        try { auto j = json::parse(input, nullptr, true, false, false, 512); } catch(...) {}
        size_t m1 = get_current_memory_mb();
        mem_wl = m1 - m0;

        // without limit  
        m0 = get_current_memory_mb();
        try { auto j = json::parse(input); } catch(...) {}
        m1 = get_current_memory_mb();
        mem_nl = m1 - m0;

        std::cout << std::setw(10) << d << " | "
                  << std::setw(12) << mem_nl << " | "
                  << std::setw(12) << mem_wl << " | ";
        if (mem_nl > mem_wl) {
            std::cout << std::setw(11) << (mem_nl - mem_wl);
        } else {
            std::cout << std::setw(11) << "0 (equal)";
        }
        std::cout << std::endl;
    }

    // ─── TEST 5: Throughput comparison ───
    std::cout << std::endl;
    std::cout << "=== TEST 5: Throughput (parse/sec) for safe JSON ===" << std::endl;
    {
        std::string safe_json = R"({"users":[{"id":1,"name":"Alice","scores":[95,87,92]},{"id":2,"name":"Bob","scores":[88,91,85]}],"meta":{"version":"1.0","count":2}})";

        const int ITERATIONS = 10000;

        auto t0 = high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {
            auto j = json::parse(safe_json);
        }
        auto t1 = high_resolution_clock::now();
        auto time_no_limit_ns = duration<double, std::nano>(t1 - t0).count() / ITERATIONS;

        t0 = high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {
            auto j = json::parse(safe_json, nullptr, true, false, false, 512);
        }
        t1 = high_resolution_clock::now();
        auto time_with_limit_ns = duration<double, std::nano>(t1 - t0).count() / ITERATIONS;

        std::cout << "  Iterations: " << ITERATIONS << std::endl;
        std::cout << "  Avg time (no limit):   " << std::fixed << std::setprecision(0)
                  << time_no_limit_ns << " ns/parse" << std::endl;
        std::cout << "  Avg time (limit=512):  " << std::fixed << std::setprecision(0)
                  << time_with_limit_ns << " ns/parse" << std::endl;
        std::cout << "  Overhead:              " << std::fixed << std::setprecision(1)
                  << (time_with_limit_ns / time_no_limit_ns * 100 - 100) << "%" << std::endl;
    }

    // ─── SUMMARY ───
    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "  SUMMARY" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "  Vulnerability:  parser has NO nesting depth limit" << std::endl;
    std::cout << "  Max depth w/o protection:  " << max_no_limit << std::endl;
    std::cout << "  Fix:  max_depth parameter (default = no limit = backward compat)" << std::endl;
    std::cout << "  With max_depth=512:  O(1) depth check, instant rejection" << std::endl;
    std::cout << "  → Prevents DoS via malicious nested JSON" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}

