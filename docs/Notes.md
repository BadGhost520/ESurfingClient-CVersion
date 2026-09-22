# 代码里删掉的"为什么"——集中记在这里

这份文档收的是源码里原本以注释形式存在、但按"只保留函数说明与函数内步骤注释"的规则
清理掉的**约束与踩坑记录**。它们不是背景故事，而是"改错了会静默出问题"的东西，所以
从代码里搬到这里集中保存。改到相关代码前先看一眼。

## 平台与编译器

- **`winsock2.h` 必须先于 `windows.h` 引入**（`src/control/ControlInternal.h`）。
  Windows 下反了会和 `windows.h` 里的 winsock1 冲突。
- **`SimEvp.h` 与 `<openssl/evp.h>` 互斥**（`include/utils/sim/SimEvp.h`）。
  该头文件是 `evp.h` 的替代品，不能与它同时包含；libcurl 仍链 libcrypto 时链接层面
  不冲突，靠的是头文件里的名称重映射。
- **枚举成员名要避开 Windows 宏**（`src/clients/dialer/DialerInternal.h` 的 `WaitResult`）。
  `windows.h` 定义了 `WAIT_FAILED`（`((DWORD)0xFFFFFFFF)`）等同名宏，而该模块会经
  `NetClient.h` → `curl.h` → `winsock2.h` 把 `windows.h` 带进来，成员名撞上就会被宏替换掉，
  报 `expected identifier before '(' token`。Linux 上不出现，只在 Windows 构建时炸。
- **线程间共享状态刻意只用 32 位量**（`src/utils/Watchdog.c`）。
  OpenWrt 路由器多是 32 位 MIPS，64 位量跨线程读写会被撕裂，可能算出错误超时。
  写入顺序是"先写预算、再自增序号"；读侧先读序号、再读预算、最后确认序号没变。

## 配置

- **默认参数必须与 `esurfingclient/files/etc/config/esurfingclient` 及 `s_default_cfg` 三处一致**
  （`src/config/ConfigDefaults.c`）。缺哪个参数，用户下次打开配置文件就会看到它被补上。
- **`s_cfg_log_dir` 只存配置里的原样文本**（定义在 `src/utils/LogCore.c`，声明在 `src/utils/LoggerInternal.h`）。
  它是给页面回显用的，不能回显解析后的绝对路径 —— 否则用户改一次配置就被写成一长串路径。

## 日志

- **OpenWrt 默认日志目录选 `/var/log/esurfing`**（`src/utils/LoggerDir.c`）。
  因为 `/var/log` 是 tmpfs：重启即清、不磨闪存、小容量设备不会被日志占满。
  配置里的 `log_dir` 同样生效（想把日志放 U 盘时用）。
  ⚠️ `files/etc/init.d/esurfingclient` 与 LuCI 的日志页都拿这个值兜底，改要一起改。
- **单条日志必须只用一次 `write` 写出**（`src/utils/LogCore.c`，`LOG_LINE_MAX` + 静态断言）。
  多进程共用同一个 `run.log`，分多次写会让不同进程的行互相穿插。
- **查询模式（`--print-log-dir` / `--list-accounts`）一行都不落盘，改写到 stderr**
  （`src/utils/LogControl.c` 里的 `set_logger_query_mode` + `LogCore.c` 的 `s_query_mode`）。这两个模式必须调 `load_cfg()` 才知道答案，而解析配置一定会写日志；
  那几行落进 `run.log` 后，OpenWrt 的 init 脚本会把这文件当成上一轮运行的日志归档走
  （它的判断就是"`run.log` 非空就归档"）—— 没有旧日志时凭空多出一个只有查询输出的 `.log`，
  有旧日志时归档里混进这几行。之所以是"改写 stderr"而不是"什么都不写"：配置有问题时得让人
  看见原因；stdout 要留给路径/账号列表，而 init 脚本调用这两个查询时都带了 `2>/dev/null`。
  查询模式比日志等级更硬：`load_cfg()` 会用配置里的 `log_lv` 调 `set_logger_level()`，
  查询模式必须在那之后依然生效。

## 看门狗

- **为什么需要它**（`include/utils/Watchdog.h`）：外部监管者（procd / systemd / SCM）只能看到
  "进程退出了没有"。进程卡死（死锁、无超时阻塞调用、绕不出来的死循环）时它照样活着，
  监管者认为一切正常、永远不会重启。OpenWrt 上尤其明显：那边连监管进程都没有，只有 procd
  在看进程在不在。
- **绝不能做成"多久没打日志就重启"**：本程序有大量合法的长时间静默（例如"不在允许时段，
  等待 3600000 毫秒后重新检查"），那样会误杀正常等待。判定依据是主循环**主动声明**
  （`watchdog_pet(30000)` = 我活着，接下来最多 30 秒不会再来打卡），只有超过声明上限加余量
  仍未打卡才判卡死。
- **判定卡死后 `_exit(1)`，不能调 `shut()`**：`shut()` 会 join 线程，可能正好卡在同一个死锁上；
  退出后交给外部监管者按 respawn 策略拉起。
- **"当前线程是不是看门狗自己"这个标记必须有**（`src/utils/Watchdog.c`，线程局部变量）：
  看门狗线程自己也要睡眠，而 `sleep_ms()` 会按睡眠时长打卡 —— 不排除自己，就等于看门狗给自己
  打卡、序号每个 tick 都在变，"有新打卡"分支永远成立，`watchdog_fire` 成了不可达代码。
  表现是"装了看门狗但卡死依然没人管"，且不报任何错。用线程局部量是因为只有本线程该被排除。

## 进程与控制通道

- **控制通道协议**（`src/control/ControlInternal.h`）：

  | 请求（一行 JSON） | 响应（一行 JSON） |
  |---|---|
  | `{"cmd":"status"}` | `{"ok":true,"data":{...}}` |
  | `{"cmd":"restart_auth"}` | `{"ok":true}` |
  | `{"cmd":"apply_config"}` | `{"ok":true}` |
  | `{"cmd":"shutdown"}` | `{"ok":true}` |

  进程拆分后 Web 进程既看不到认证状态、也不能直接改认证线程的运行时状态，所以要这条只监听
  回环的控制通道，鉴权靠监管者下发的令牌。**刻意不放配置读写**：两个进程读同一份配置，
  Web 进程保存配置也直接写文件，通道只负责运行时状态与动作，协议面最小、最不容易出错。
- **退出请求只由信号处理函数置位**（`src/supervisor/SupervisorInternal.h` 的 `s_stop_requested`）：
  信号处理函数里不能做 join / 打日志 / rename / exit 这些不是 async-signal-safe 的事，
  真正的关闭动作由各角色的主循环来做。
- **模块内共享状态的定义只放一处，头文件里只放 `extern` 声明**
  （`src/supervisor/SupervisorState.c`、`src/utils/LogCore.c` 等）。若把可变状态定义在头文件里，每个 `.c` 会拿到
  自己的一份副本，`s_child_count` 之类的计数就各算各的，行为与拆分前不同。
- **`_Thread_local` 的限定符在声明与定义两侧都必须带**（`src/clients/net/NetClient.c` 的
  `s_request_url`，声明在 `NetClientInternal.h`）。少了它就不是线程局部对象，`header_cb` 拼 Location
  时的基准地址会串到别的线程去。

## iOS ZSM 包格式（逆向参考）

`src/cipher/algo/ios/ios_zsm.c` 解析的 `IZsmModLoad`（`sub_10007131C`）布局：

```
[3-byte hdr][u8 len1][str1][u8 len2][str2=AID]
[5-byte LZMA props][u32le packed: top nibble type==2, low 28 bits unpacked size]
[TEA ciphertext...]
```

- TEA key：32 字节 ASCII，两段 16 字节，各 32 轮解密，8 字节一组
  （`Rirn53a;feb#UXES5ZrRBTGmYwml:fRt`）
- LZMA 解压后的布局：`buf[0xFA]` = IV 长度，`buf[0xFC]` = key 长度，
  `buf[0xFF]` = key 偏移基址（key 在 `buf[buf[0xFF]+1]`），JS 源码在 `buf+0x103`
- JS 全局量：`cdckey` / `cdciv` / `cdy(type, mode, key, iv, data)`；
  type 通常写在 `var codex = 0xNN;` 再 `cdy(codex, ...)`；
  `type < 16` → oCode(1-9)，`type >= 16` → nCode（尚未移植）

## 前端与后端的同步点

前端的注释清理掉了，但下面这些"两边必须一致"的约束依然成立：

- **内置网页界面（`app/portal/assets/js/main.js`）里的常量要和后端常量对齐**：
  `DEFAULT_CONN_TIMEOUT` / `DEFAULT_OP_TIMEOUT` / `DEFAULT_WEB_PORT` / `MAX_TIME_WINDOWS`
  对齐 `include/states/States.h`；默认日志目录对齐 `include/utils/Logger.h` 的 `DEFAULT_LOG_DIR`（`"./"` 表示
  程序所在目录）；日志目录长度按后端 `PATH_MAX` 校验，超了会退回默认目录。
- **认证通道取值统一用 `windows/linux/android/ios/macos`**：前端下拉框、`index.html`、
  后端 `parse_channel_json` 三处一致（后端默认值是 Android）。
- **LuCI 页面（`rootfs/www` 与 `rootfs-legacy`）**：默认配置与后端 `s_default_cfg` 是同一套字段；
  `web_port` / `web_external_acc` 是桌面端参数，OpenWrt 版不带网页服务，界面上不提供但字段保留
  ——配置格式两个平台共用，复位时写出去的也必须是完整一套；日志目录规则与 `LoggerDir.c` 的
  `resolve_log_dir` / `get_log_dir` 一致（配置里的 `log_dir` 是**基目录**，日志在它下面的
  `logs` 里；没写或写成 `.` / `./` 时用 `/var/log/esurfing`；相对路径也按它解析，OpenWrt 上
  程序装在只读的 `/usr/bin` 里，不按程序目录解析）。
- **`app/portal/index.html` 里有两处是给打包脚本用的标记**，改动时要一起看
  `scripts/build-portal.sh`：
  - daisyUI 主题那一行**必须独占一行**，打包时整行删除（脚本按行匹配删）；
  - `class-keeper` 那段是让开发模式的 Tailwind 浏览器版生成运行时才用到的类，打包后由
    `input.css` 负责。
- 内置网页界面与 LuCI 界面的**状态指示灯颜色都交给 Alpine 的 `:class` 绑定**，不要直接操作
  `classList`，两者会互相覆盖。
- `main.js` 对 `/api/logs`、`/api/status/sys`、`/api/restartAuth` 这三个较新接口做了降级处理：
  旧后端返回 404 时功能自动降级并提示，不报错崩溃。

## 仓库与打包约定

- **包目录自包含**：`esurfingclient/`、`luci-app-esurfingclient/` 会被整个拷进 OpenWrt SDK
  （`cp -r <包> openwrt-sdk/package/`），因此任何 `../` 形式的引用（`../LICENSE`、
  跨包读版本号）在 SDK 里都会指到别处，而本地看着一切正常。
- **`files/etc/init.d/esurfingclient` 必须是 LF**：`core.autocrlf=true` 时这个文件在 Windows 上
  检出会变成 CRLF，而带 `\r` 的 `#!/bin/sh` 在路由器上跑不起来（报 "not found" —— 解释器
  路径后面跟着一个 `\r`）。它平时在 Windows 工作区里是 LF，很难注意到，直到某次重新检出。
  `.gitattributes` 里按角色钉了 `**/etc/init.d/*`、`**/*.init`、`*.sh`。
- **版本号唯一源是 `esurfingclient/app/CMakeLists.txt` 的 `set(PROGRAM_VERSION_*)` 四行**，
  改完跑 `scripts/sync-version.sh` 分发到两个包的 Makefile 与两处 LuCI 页面（那些是生成物）。
- **`luci-app` 的 `/tmp/luci-staging` 中转是故意的**：postinst 在目标机上探测属于哪一代 LuCI，
  再决定装 JS 那套还是 Lua/HTM 那套，别"顺手清理"。
- **portal 的网页资源**：`app/portal` 里是源（`index.html` 引用 CDN、`input.css` 是 Tailwind
  输入），发布用的 `assets/css/tailwind.css`、`assets/js/alpine.js`、`daisyui.mjs` 由
  `scripts/build-portal.sh` 生成，不参与版本管理。
