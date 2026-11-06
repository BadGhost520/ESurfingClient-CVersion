# AGENTS.md — 给接手这个仓库的 AI agent

> 本文件是**项目级通用指令**，面向任何 AI 编码工具（Claude Code / Codex / Cursor / Copilot / DSH 等），不依赖某个 IDE、某台机器或某个私有环境。
> 它只描述**仓库里真实存在的东西**和**改代码前必须知道的约束**。动手顺序建议：本文件 → `docs/` 下对应文档 → 目标文件附近的注释。

## 1. 这是什么

`ESurfingClient-CVersion-WithCipher` —— 中国电信天翼校园网（ESurfing）认证客户端的**纯 C11** 实现，带密文认证。
覆盖 Windows / Linux / macOS / OpenWrt：桌面端是 **supervisor 多进程**模型 + 自带 Web 控制面板；OpenWrt 端是 procd 托管的多实例 + LuCI 页面。

## 2. 仓库里有什么、没有什么

### 2.1 在仓库里（可直接读、直接构建）

| 路径                                     | 作用                                                                                 |
|------------------------------------------|--------------------------------------------------------------------------------------|
| `esurfingclient/app/CMakeLists.txt`      | CMake 工程入口，**版本号唯一真相源**（`PROGRAM_VERSION_*`）                          |
| `esurfingclient/app/src/` `include/`     | 第一方源码与头文件（结构与下文"模块"一致）                                           |
| `esurfingclient/app/third_party/`        | vendored 依赖：cJSON / mongoose / 7z（不需要联网拉取）                               |
| `esurfingclient/app/portal/`             | 桌面网页源码（Alpine + daisyUI）                                                     |
| `esurfingclient/files/`                  | OpenWrt 包文件：`etc/config/esurfingclient`（默认配置）、`etc/init.d/esurfingclient` |
| `esurfingclient/Makefile`                | OpenWrt 包 Makefile（CI 会把整个目录拷进 SDK）                                       |
| `luci-app-esurfingclient/rootfs/`        | 新版 LuCI（JS 视图，21.02+）                                                         |
| `luci-app-esurfingclient/rootfs-legacy/` | 旧版 LuCI（Lua/HTM，18.06 之前）                                                     |
| `docs/`                                  | Compile / Desktop / Notes / OpenWRT / OpenWRT_mwan3 / OpenWRT_targets / Q&A          |
| `ci/`                                    | 构建输入：`openwrt/all.config`、`toolchains/mingw.cmake`                             |
| `scripts/`                               | `build-portal.sh`、`sync-version.sh`                                                 |

### 2.2 不在仓库里（被 `.gitignore`，fork 后要自己生成）

- 构建目录：`build/`、`cmake-build-*/`、`out/`、`openwrt-sdk/`
- 编译产物：`*.o` `*.obj` `*.a` `*.exe` `*.dll` `*.so` `*.ipk` 等
- 运行时文件：`logs/`、`*.log`、**用户配置 `ESurfingClient.json`**（程序首次启动自动生成）
- **portal 的生成资源**：`portal/assets/css/`、`portal/assets/js/alpine.js`、`portal/daisyui.mjs`、`portal/assets/traffic.json`
  → 由 `scripts/build-portal.sh <portal源目录> <输出目录> <tailwindcss可执行文件>` 生成（下载 alpine/daisyui + tailwind 编译 + CDN 本地化）

所以：**不要假设 `build/`、`logs/`、`ESurfingClient.json` 存在**；也不要把它们提交进仓库。

## 3. 从零构建

前置：C11 编译器、CMake ≥ 3.16、libcurl（优先静态库，`find_package(CURL)`）、make 或 ninja。
分平台详细步骤见 [`docs/Compile.md`](docs/Compile.md)；CI 的动作是权威参考。

```bash
# 桌面构建（默认）
cmake -S esurfingclient/app -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 只验证 OpenWrt 代码路径能否编译/链接
cmake -S esurfingclient/app -B build-openwrt -D__OPENWRT__=1 -DCMAKE_BUILD_TYPE=Release
cmake --build build-openwrt -j
```

要点：

- `-D__OPENWRT__=1` 走 OpenWrt 分支（**不编译** `webserver/` 与 `supervisor/`，只留单进程多线程主体）。
- 改版本号：只改 `esurfingclient/app/CMakeLists.txt` 的 `PROGRAM_VERSION_*`，再跑 `scripts/sync-version.sh` 分发到各包与页面；不要手改散落各处的版本字符串。
- 前端改完至少做语法检查：
  - 普通 JS：`node --check <file>`；
  - LuCI 视图是 `'require view'` 模块格式：去掉首部 `'require ...'` 行、把 `return view.extend(` 换成 `const __view = (`，再 `node --check`。
- 冒烟测试：在**临时目录**放最小 `ESurfingClient.json`（`enabled:true` + 假账号 + 高位端口），启动后 `curl 127.0.0.1:<port>/api/status/sys`，再 `kill -TERM`；不要把测试配置留在仓库里。
- 提交前保证构建**零 warning**（`-Wformat-truncation` 这类也要修）。

## 4. 模块分工

| 模块          | 职责                                                                                                                        |
|---------------|-----------------------------------------------------------------------------------------------------------------------------|
| `core/`       | 全局状态（`States.c` 是唯一的全局变量定义处）、Shutdown、日志控制                                                           |
| `config/`     | 配置加载/补全/写回（`Config.c`、`ConfigDefaults.c`、`ConfigAccounts.c`、`ConfigTimeWindow.c`）                              |
| `dialer/`     | 认证主流程：`DialerAuth` / `DialerClient` / `DialerNet` / `DialerLocation` / `DialerServer` / `DialerSession` / `ErrorCode` |
| `net/`        | curl 封装（`NetClientCurl.c`）、联网探测（`CheckNetwork.c`）、网速采样（`NetTraffic.c`）                                    |
| `cipher/`     | 认证报文加解密算法                                                                                                          |
| `control/`    | Web 进程 ↔ 认证进程的控制通道（默认端口 8890，占用自动顺延）                                                                |
| `supervisor/` | 桌面监管进程：拉起/回收 auth 与 web 子进程                                                                                  |
| `webserver/`  | HTTP API、静态页、热点控制（`HotspotControl.c`）                                                                            |
| `utils/`      | 日志、文本、平台（`NetAdapters` / `Service` / `ChildRun`）、运行时                                                          |

## 5. 运行形态（改之前必须知道）

- **桌面**：不带参数 = `--role supervisor`，它拉起 N 个 `--role auth --account N` + 1 个 `--role web`。
  单进程主体整段在 `#ifdef __OPENWRT__` 里，**只有 OpenWrt 构建才编进去**。
- **OpenWrt**：没有监管进程。init 脚本按账号 `procd_open_instance "account$idx"` 起 `--role auth --account $idx`。
- 桌面三个子进程共用同一份日志文件与同一个控制台；跨进程写控制台必须整行原子写。

## 6. 认证链路（别凭感觉改）

```
generate_204 并发探测（固定 IP 多路，不发 DNS）
  → 302 的 Location 记进 last_location
  → select_auth_server() 并发测速选服务器
  → auth.cgi
  → ticket.cgi?wlanuserip=<client_ip>&wlanacip=<ac_ip>   ← 这两个参数必须带
  → keep.cgi（心跳，间隔由服务端返回的 keep_retry 决定）
  → term.cgi（登出）
```

- 探测**不允许**加回 DNS 那一路（校园网 DNS 常解析失败甚至被劫持）；固定 IP 多路并行，谁先给 204/301/302 用谁。
- `last_location` 只能从 302 响应头学到；`get_last_location()` 开头会先踢一脚 `http://connect.rom.miui.com/generate_204`。
- `ostag` 按通道硬编码（Windows 11 / Linux / Android 14 / iOS 17.0 / macOS 14.4），不要随机。
- 报文 `<gwip>` 用默认网关，不是 AC 地址。

## 7. 配置生命周期（2026-10 重构后，别回退）

- 配置文件是**单一 JSON**：`enabled` / `web_external_acc` / `log_lv` / `log_dir` / `conn_timeout` / `op_timeout` / `web_port` / `probe_hosts` / `accounts`。
- `load_cfg()`：`complete_cfg()` 补缺失键 → 解析 → **每次加载都原子写回**（临时文件带 pid + rename）。
- 写回顺序是**硬约束**：
  - 全局字段与 `probe_targets_parse` 必须在 `#ifdef __OPENWRT__` 分支 realloc + `memset` **之后**、`--account` 挑选 **之后**、`g_prog_cnt` 赋值之后；
  - `accounts` 一律从 `cfg_json` 用 `cJSON_Duplicate` 拷贝，**不得从 `g_prog_status` 重建**（`--account N` 进程只加载自己那个，重建会删掉别人的账号）；
  - `cJSON_Delete(cfg_json)` 必须在 `save_cfg_from_cfg(cfg_json)` **之后**。
- **保存 = 只写文件**（任何进程都不重读）。
- **应用 = 认证进程整个退出重拉；supervisor / web 热重载**（靠 `<配置路径>.apply` 标记文件传播：`cfg_request_apply()` / `cfg_take_apply_request()`）。
- Web 端：`POST /api/saveConfigs` 只保存；`POST /api/applyConfigs` 带 `{"apply":true}` 才应用。
- 默认值以 `esurfingclient/files/etc/config/esurfingclient` 为准（`s_default_cfg`、`DEFAULT_CHANNEL=1`、`DEFAULT_CONN_TIMEOUT=10`、`DEFAULT_OP_TIMEOUT=15`、`DEFAULT_LOG_DIR="./"` 等宏必须与它一致）。

## 8. 探测目标（probe_hosts）

- 限制：单域名最多 **4** 个 IP（`PROBE_IP_MAX`）、最多 **8** 条（`PROBE_HOST_MAX`）、探测并发 **16** 路（`PROBE_MAX`，认证服务器固定占 2 路 → 实际可用 14 路）。
- 超出时：前端（桌面网页 / LuCI）保存前拦截；后端解析时打 WARN，**不静默截断**。
- 桌面网页与两版 LuCI 的配置页都把「超时 / 日志目录 / 端口 / 外部访问 / 探测目标」收在**高级选项**折叠里（默认收起，纯 UI 状态，不写配置）。

## 9. 多 WAN 与 mark（OpenWrt）

- 每账号一个 mark（0x100 / 0x200 / …），`nftables` 按 mark DNAT 到不同出口。
- **凡是代表某个账号发起的请求都必须绑 mark**：走 `NetClientCurl.c` 的 `open_socket_callback()`（`setsockopt(SO_MARK)`）。
- `open_socket_callback` 里必须先判 `tl_thread_idx >= 0`：Web 进程的后台探测线程是 -1，越界会崩。
- 新增"替某个账号发请求"的代码时，检查它有没有走这条路；自己写 socket 就自己 `SO_MARK`。

## 10. Web / 控制通道

- Web 线程是**单线程**（mongoose `mg_mgr_poll`），任何接口都不能阻塞；新增阻塞接口前先想清楚会不会让 1 秒一次的 `/api/traffic` 排队。
- 控制通道连接超时 300ms、读超时 1000ms；端口被占自动顺延（`supervisor_pick_control_port`）。
- `web_port` **不参与**自动顺延，冲突直接报错。
- 页面"保存并应用"后靠响应里的 `bootId` 变化判断重启完成并自动刷新，不要用固定延时。

## 11. 日志

- 级别：FATAL / ERROR / WARN / INFO / DEBUG / TRACE；`log_lv` 0–6，默认 4。
- **INFO 只留用户必须看见的**：认证/联网状态、banner 5 行、Web 访问地址、配置路径/首次创建指引、用户操作反馈、退出码。
- 其余（模块启停、配置加载细节、子进程拉起/停止、关闭步骤）= DEBUG。
- 状态类提示用"变化才打"（`log_network_state_change`），不要每轮刷屏。
- Windows 控制台输出必须整行一次写（`LogCore.c` 的命名互斥），不要退回 `printf`。

## 12. 代码风格

- C11，`snake_case`，文件内辅助函数一律 `static`；注释用中文，`/** */` 写函数说明。
- 不用 C++ 特性；跨平台差异用 `#ifdef _WIN32` / `__linux__` / `__OPENWRT__` / `__APPLE__`。
- 内存/句柄显式释放；错误路径先清理再返回；新增缓冲用 `sizeof(实际数组)` 而不是拍脑袋的 `PATH_MAX`。
- 改动后至少通过桌面与 OpenWrt 两套构建，且零 warning。

## 13. 前端约定

- `esurfingclient/app/portal/`：Alpine.js 组件 + `CONFIG_FIELDS` 表驱动配置项；新增配置项要同时改 `CONFIG_FIELDS`、`buildPayload` 与页面模板。
- `luci-app-esurfingclient/rootfs/`：LuCI 视图，`E()` 构建 DOM；`normalizeProbeHosts` 缺省返回内置默认 2 组。
- `rootfs-legacy/`：Lua 模板 + 内嵌 JS；**两版 LuCI 都要同步改**，只改一边会出现新旧界面行为不一致。
- 表格统一写法（以账号表格为样板）：`table cbi-section-table` / `thead class='thead'` / `tr class='tr cbi-section-table-titles'` / `th class='th'` / `tbody class='tbody'`，操作列宽 135px，添加按钮放表格外。

## 14. 文档与提交

- `CHANGELOG.md`：CRLF，`### YYYY.MM.DD (标题)` + 每条 10~20 字，只写"做了什么"，不写原因/细节/验证数据。
- 行为变化要同步 `docs/` 与 `README.md`。
- **只提交源码与文档**：不要提交构建目录、日志、`ESurfingClient.json`、portal 生成资源（见 §2.2）。
- 临时脚本/日志放系统临时目录或 `TEMP/`（已 ignore），不要散落在仓库根。

## 15. 禁止事项

- 不要回退 2026-10 之后的重构：配置生命周期、探测目标可配置、单实例锁、mark 绑定、日志级别降噪、默认配置对齐。
- 不要删除固定 IP 探测那几路，不要加回 DNS 探测。
- 不要把 `HotspotControl.c` 挪出主源清单（OpenWrt 链接需要它的桩）。
- 不要手工改各处的版本号字符串（走 `sync-version.sh`）。
- 不要在 Windows 上用 `chcp` 修控制台乱码（已由 `console_utf8_init()` 处理）。
- 不要凭猜测改认证报文结构：先用 `docs/Notes.md` 与代码里的注释确认，再动 `Xml.c` / `DialerAuth.c`。

---

本文件只记"改之前必须知道"的约束，不代替源码。有疑问先读对应 `docs/*.md` 与代码注释；如果某条约束与代码现状冲突，以代码为准并在 PR 里说明。
