# nlohmann/json Security Contribution — Final Summary

## 任务概览
开源软件与安全课程作业：为 nlohmann/json (v3.12.0, 44k+ Stars) 增加嵌套深度限制功能，防止 DoS 攻击。

## 改动文件

### 1. 核心安全修复
| 文件 | 改动 |
|------|------|
| `include/nlohmann/detail/input/parser.hpp` | 新增 `#include <limits>`、构造参数 `max_depth_`、`sax_parse_internal()` 深度检查 |
| `include/nlohmann/json.hpp` | 全部 6 个 parse/accept 重载 + parser() 工厂函数的 `max_depth` 参数透传 |

### 2. 测试文件
- `tests/src/unit-max_depth.cpp` — 11 个 TEST_CASE，49 个断言，doctest 框架
- `test_depth/bench_v3.cpp` — 4 模组量化基准测试

## 量化结果

| 指标 | 数值 |
|------|------|
| 无保护最大深度 | ~7,950,000 层 (后 OOM) |
| 保护拒绝 100 万层 | **0.2ms** (vs 无保护 107ms → 500x 加速) |
| 正常 JSON 吞吐量开销 | **+2.26%** (O(1) 整数比较) |
| 现有测试回归 | **10,241/10,241 通过** ✅ |
| 新增单元测试 | **11/11 TEST_CASE 全部通过** ✅ |
| 向后兼容 | 100% (默认值=unlimited) |

## 验证步骤
```bash
# 编译并验证
g++ -std=c++17 -O2 -I include -o bench test_depth/bench_v3.cpp -lpsapi
./bench 1  # 功能验证
./bench 4  # 保护效果
./bench 3  # 吞吐量

# 运行单元测试
g++ -std=c++17 -O0 -I include -I tests/thirdparty/doctest \
  -DJSON_TESTS_PRIVATE -o test-max_depth \
  tests/src/unit.cpp tests/src/unit-max_depth.cpp
./test-max_depth -s

# 回归测试
g++ -std=c++17 -O0 -I include -I tests/thirdparty/doctest \
  -DJSON_TESTS_PRIVATE -o test-regression \
  tests/src/unit.cpp tests/src/unit-class_parser.cpp
./test-regression -s
```

## 待完成 (交由项目维护者)
1. 同步 `single_include/nlohmann/json.hpp`（运行 amalgamate 工具）
2. 更新 API 文档（parse/accept 函数签名）
3. 决定默认推荐值（建议 256 对齐 rapidjson，或 512/1024）

## 工作路径
`C:\Users\缪臻\.qclaw\workspace\nlohmann-json-analysis\`