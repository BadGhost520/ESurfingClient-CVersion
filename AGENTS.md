# AGENTS.md — 给接手这个仓库的 AI agent

> 项目级通用指令（任何 AI 编码工具都适用）。只记「改之前必须知道」的约束，细节看 `docs/` 与代码注释。

## 1. 这是什么

天翼校园网（ESurfing）认证客户端的纯 C11 实现，约 2.3 万行：桌面端 supervisor 多进程 + Web 面板，OpenWrt 端 procd 多实例 + LuCI。

## 2. 目录

- `esurfingclient/app/`：CMake 工程（`CMakeLists.txt` 是版本号唯一真相源）、`src/` `include/`、`portal/`（桌面网页）、`third_party/`（cJSON/mongoose/7z，已 vendored）
- `esurfingclient/files/`：OpenWrt 包文件（init.d、UCI 配置、默认 JSON）
- `luci-app-esurfingclient/`：`rootfs/`（新 LuCI）+ `rootfs-legacy/`（旧 LuCI）
- `docs/`：Compile / Desktop / Notes / OpenWRT / Q&A
- `ci/` `scripts/`：构建输入与辅助脚本

不在仓库（被 gitignore，fork 后自己生成）：构建目录、产物、`logs/`、`ESurfingClient.json`、portal 的 `assets/css` / `alpine.js` / `daisyui.mjs`（用 `scripts/build-portal.sh` 生成）。

## 3. 构建

```bash
cmake -S esurfingclient/app -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
# 只验证 OpenWrt 代码路径
cmake -S esurfingclient/app -B build-openwrt -D__OPENWRT__=1 && cmake --build build-openwrt -j
```

依赖 libcurl（`find_package(CURL)`）。版本号只改 `CMakeLists.txt` 的 `PROGRAM_VERSION_*`，再跑 `scripts/sync-version.sh`。前端改完 `node --check`（LuCI 模块先去掉 `'require ...'` 行、把 `return view.extend(` 换成 `const __view = (`）。提交前零 warning。

## 4. 运行形态

- 桌面：无参数 = supervisor，拉起 N×`--role auth --account N` + 1×`--role web`；单进程主体只在 `__OPENWRT__` 下编译。
- OpenWrt：init 脚本按账号起 `--role auth --account N --config-dir <dir>`，没有 supervisor / web。

## 5. 认证链路

`generate_204` 并发探测（固定 IP 多路，**不发 DNS**）→ 302 记 `last_location` → 测速选服务器 → `auth.cgi` → `ticket.cgi?wlanuserip=&wlanacip=` → `keep.cgi` 心跳 → `term.cgi` 登出。
`ostag` 按通道硬编码；`<gwip>` 用默认网关。**不要加回 DNS 探测，不要删固定 IP 那几路。**

## 6. 配置（重点）

- 单一 JSON：`enabled` / `web_external_acc` / `log_lv` / `log_dir` / `conn_timeout` / `op_timeout` / `web_port` / `probe_hosts` / `accounts`。
- `load_cfg()`：`complete_cfg()` 补缺键 → 解析 → **每次加载原子写回**（tmp + rename）。
- 写回顺序硬约束：全局字段与 `probe_targets_parse` 必须在 OpenWrt realloc + `--account` 挑选 + `g_prog_cnt` 之后；`accounts` 从 `cfg_json` 用 `cJSON_Duplicate` 拷贝（不得从 `g_prog_status` 重建）；`cJSON_Delete(cfg_json)` 在 `save_cfg_from_cfg()` 之后。
- **保存 = 只写文件；应用 = auth 进程退出重拉，supervisor / web 热重载**（靠 `.apply` 标记文件）。
- Web：`/api/saveConfigs` 只保存；`/api/applyConfigs` 带 `{"apply":true}` 才应用。
- **OpenWrt 配置目录由 UCI 决定**：`/etc/config/esurfingclient` 是 `config global 'config'` + `option dir`，JSON 在 `<dir>/ESurfingClient.json`，init 脚本用 `--config-dir` 传给程序（程序侧不解析 UCI）。访问写法统一用 `esurfingclient.config.dir`。**别把 JSON 放回 `/etc/config`**（`uci` 解析失败会连带 `uci set` 报错）。
- 默认值以 `files/etc/esurfingclient/ESurfingClient.json` 与 `s_default_cfg` 为准。

## 7. 探测目标

单域名 ≤ 4 个 IP（`PROBE_IP_MAX`）、≤ 8 条（`PROBE_HOST_MAX`）、总并发 16（`PROBE_MAX`，认证服务器占 2 → 可用 14）。超限时前端拦、后端 WARN，不静默截断。配置页的高级参数收在「高级选项」折叠里。

## 8. 多 WAN mark

每账号一个 mark（0x100…），nftables 按 mark DNAT。**凡代表某个账号的请求都要绑 mark**（`open_socket_callback` → `SO_MARK`），且先判 `tl_thread_idx >= 0`（Web 线程是 -1，越界会崩）。

## 9. Web / 控制通道

Web 线程是单线程（mongoose），**接口不能阻塞**（否则 `/api/traffic` 排队）。控制通道连接 300ms、读 1000ms 超时；端口被占自动顺延，`web_port` 不顺延。重启完成看 `bootId` 变化，别用固定延时。

## 10. 日志

级别 FATAL / ERROR / WARN / INFO / DEBUG / TRACE，`log_lv` 0–6 默认 4。**INFO 只留用户必须看见的**（认证/联网状态、banner、访问地址、配置路径、操作反馈、退出码），其余 DEBUG。状态类提示用「变化才打」。Windows 控制台整行一次写。

## 11. 代码风格

C11 / `snake_case` / 文件内辅助函数 `static` / 中文注释 `/** */`。跨平台用 `#ifdef _WIN32` / `__linux__` / `__OPENWRT__` / `__APPLE__`。显式释放；缓冲用 `sizeof(实际数组)`。改完至少过桌面 + OpenWrt 两套构建。

## 12. 前端

- portal：`CONFIG_FIELDS` 表驱动，新增配置项要同时改字段表、`buildPayload`、模板。
- LuCI：`E()` 构建 DOM；两版（`rootfs` + `rootfs-legacy`）**必须同步改**。
- 表格样板（账号表格）：`table cbi-section-table` / `thead class='thead'` / `tr class='tr cbi-section-table-titles'` / `th class='th'` / `tbody class='tbody'`，操作列 135px，添加按钮放表格外。

## 13. 文档与提交

- `CHANGELOG.md`：CRLF，`### YYYY.MM.DD (标题)`，每条 10~20 字，只写「做了什么」。
- 行为变化同步 `docs/` 与 `README.md`。
- 只提交源码与文档，不提交构建产物 / 日志 / 用户配置 / portal 生成资源。

## 14. 禁止事项

- 别回退 2026-10 后的重构：配置生命周期、探测目标可配置、单实例锁、mark 绑定、日志降噪、默认配置对齐。
- 别删固定 IP 探测、别加回 DNS 探测。
- 别把 `HotspotControl.c` 移出主源清单。
- 别手改版本号字符串（走 `sync-version.sh`）。
- 别用 `chcp` 修控制台乱码（已有 `console_utf8_init()`）。
- 别凭猜改认证报文结构，先看 `docs/Notes.md`。
