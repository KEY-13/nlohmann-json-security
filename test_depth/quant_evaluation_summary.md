# nlohmann/json Security Contribution — Quantitative Evaluation Summary

## 漏洞定性
**项目**: nlohmann/json v3.12.0 (44k+ GitHub Stars)
**漏洞类型**: DoS (拒绝服务) — 递归下降解析器无嵌套深度限制
**CVSS 评估**: ~5.3 (Medium, AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:L)
**攻击向量**: 攻击者提交深度嵌套 JSON (`[[[[...]]]]`)，消耗服务器内存/CPU，导致 OOM 崩溃

## 修复方法
在 `parse()` / `accept()` API 新增可选参数 `max_depth`（默认值 = `(std::numeric_limits<std::size_t>::max)()` 保持完全向后兼容）。parser 构造函数透传至 `sax_parse_internal()`，在 `begin_object`/`begin_array` 处理中检查 `states.size() >= m_max_depth`，超限抛出 `parse_error.101`。

## 量化测试结果

### 1. 崩溃深度（无保护）
- 解析器成功解析至 **~7,950,000 层**嵌套后被 OOM SIGKILL
- 在内存受限的生产环境（容器/Docker/云函数），崩溃点远低于此
- 每 100 万层消耗 ~2MB+ 堆内存

### 2. 保护效果 (max_depth=512)
| 输入深度 | 结果 | 耗时 |
|----------|------|------|
| 512 | PASS | 0.071ms |
| 1,000 | REJECT | 0.122ms |
| 1,000,000 | REJECT | **0.203ms** |

- 100 万层恶意 JSON → **0.2ms 内被拒绝** (无保护需 107ms)
- **拒绝速度提升 500 倍**

### 3. 正常场景开销 (20k 次解析)
| 模式 | 每次解析 |
|------|---------|
| 无深度限制 | 3,716 ns |
| max_depth=512 | 3,800 ns |
| **额外开销** | **+2.26%** |

### 4. 功能正确性
- ✅ 普通 JSON 解析不受影响
- ✅ 深度内 JSON 正常解析
- ✅ 超限被正确拒绝 (parse_error.101)
- ✅ accept() 正确返回 false
- ✅ 极端值 depth=0 正确拒绝任何嵌套

## 改动的源文件
1. `include/nlohmann/detail/input/parser.hpp` — 新增 max_depth_ 成员 + 深度检查
2. `include/nlohmann/json.hpp` — 所有 parse/accept/sax_parse 重载 + parser 工厂函数

## 下一步
- 编写正式单元测试 (Catch2, 融入项目 CI)
- 更新 API 文档
- 同步更新 single_include/nlohmann/json.hpp (单头文件版)
- 向 nlohmann/json 官方仓库提交 PR