# 更新日志

### 2025.11.03

1. 更新 OpenWrt-Arm64-MTFilogic-24.10.4(内核版本 6.6.110) 的 ipk 包
2. 更新 OpenWrt-Mipsel_24kc-MT7621-24.10.4(内核版本 6.6.110) 的 ipk 包

### 2025.11.10

1. 优化程序启动参数，现在 `通道选择` 参数为可选参数(默认为 pc 通道)，使用 -cpc 或 -cphone 切换相应通道

### 2025.11.14

1. 优化程序逻辑，修复了在 OpenWrt 系统中程序不能正确接收到 /etc/init.d/esurfingclient stop 的 SIGTERM 退出信号并且退出生成日志
2. 优化 ipk 包，现在更新包不会导致原配置被覆盖而需要重新填写，现在 ipk 包更新时检测到 /etc/config/esurfingclient 配置文件存在时，会生成 /etc/config/esurfingclient-opkg 配置文件而不会覆盖原配置文件
3. 修复了 OpenWrt 系统更新包时因为关闭程序和启动程序中间间隔过短导致的严重程序报错

> 关于第 2 点，就是现在 ipk 包更新不再需要重新填写个人的信息了，会自动用旧的配置

### 2025.11.16

1. 修复因为把 response_code 参数的 long 类型改成 int 类型导致的 Segmentation fault 段错误的问题
2. 尝试修复因为登录时长过久导致的 Invalid zsm data 并且无法上网的错误
3. (临时)优化日志系统，现在开启 debug 模式会让日志生成在 /usr/esurfing 目录中，而不是 /var/log/esurfing 中

### 2025.11.17

1. 优化日志系统，现在在 OpenWrt 系统开启 debug 模式会让日志生成在 /usr/esurfing 目录中，而不是在重启后就会清除的 /var/log/esurfing 目录中，其它系统仍旧
2. 将 Linux 版本做成静态链接版本，避免了因链接库版本不同导致的无法运行

### 2025.11.20

1. 全面汉化日志输出
2. 优化日志系统，为避免小容量的 OpenWrt 系统设备因为在开启 debug 模式之后容量不足，现添加一个新参数 `-s` (uci 配置则是 smallDevice 参数)，表示 smallDevice 小容量设备，默认开启，开启之后不管是否开启 debug 模式，都会将日志存储在 /var/log/esurfing 目录中，只有在同时满足 debug 模式开启和 smallDevice 模式关闭的条件下才会将日志存储到 /usr/esurfing 目录中，该参数仅对 OpenWrt 系统有效，其它系统启用无任何效果
3. 新增了一些报错提示
4. 取消 OpenWrt 系统安装 ipk 包时的配置选项保护，以保证默认配置文件能得到更新，每次更新需要重新设置配置文件，不过现在在 OpenWrt 系统中每次执行本程序，都会保存一个配置脚本在 /root/config.sh 中，以后每次更新都可以直接运行该脚本来更新自己的配置
5. 更换默认认证通道为更稳定的 `phone` 通道(因为今晚测试 `pc` 通道十分不稳定)

### 2025.11.21

1. 新增一些详细的日志报错

### 2025.12.01

1. 优化代码结构
2. 新增登录时间检测，在 1 天 23 小时 50 分钟后会自动重新认证 (服务器会在认证登录 1 天 23 小时 55 分钟后踢下线)

### 2025.12.04

1. 优化代码结构
2. 尝试修复重新认证机制的空指针错误
3. (OpenWrt 系统) 将 config.sh 的生成路径改为 /var/log/esurfing/config.sh 避免一些未知的权限问题

### 2025.12.06

1. 优化
2. 确认修复了认证机制的空指针错误

### 2026.3.16

1. 合并相同类型的包，支持版本更明确
2. 更新相应 Markdown 文档

### 2026.4.30

1. 终于完成重构代码和多播功能了喵~
2. 更新主分支到 v2 版本
3. 修复了一些 bug
4. OpenWRT ipk 包支持同一架构的全版本系统了~
5. 被蚊子咬到
6. 现在的 CMakeLists.txt 不再像以前那样要莫名依赖一个 CLion 的 cmake 文件
7. 更好更快的 github action 工作流
8. 新的认证配置文件 (JSON 文件)
9. 程序支持各种关闭方式 (包括但不限于 Windows 的直接关闭 CMD 窗口、Ctrl + C、关闭设备 (不是直接断电喔, 直接断电哪个程序都管不了))
10. 喝了一杯西瓜冰沙
11. 还有一些我也忘了~总之就是很多~

### 2026.5.4

1. 修复了登录失败重试过短的问题
2. README 和 Release 增加了理论最低支持版本和最低推荐版本说明
3. 程序内添加了版本提示
4. 程序内添加了 5 分钟提示一次的守护线程运行状态

### 2026.5.7

1. 新增原有架构 apk 包支持
2. 新增 qualcommax_ipq60xx 架构支持
3. 更新安装教程
4. 修复了认证步骤中重置参数问题 #14

### 2026.5.9

1. 修复部分 Linux 设备报 Floating point exception 错误
2. 添加了自行编译教程
3. 修复部分设备截取 location 错误问题

### 2026.5.15

1. OpenWRT 包添加配置文件保护
2. 配置文件移动到 /etc/config/, 转由程序进行检测是否启动

### 2026.5.16 (版本 v2.0.1-r1)

1. 添加 macOS 支持
2. Windows, Linux, macOS 添加安装服务, 可通过指定参数安装和卸载自启动服务
3. 修了一些奇妙 bug

### 2026.5.18 (版本 v2.0.2-r3)

1. 关于 OpenWrt 版本的超棒更新! (高举双手)
2. 被 OpenWrt 软件包管理折磨的一天
3. 使用了折中的方案去让 OpenWrt 的日志系统正常运行
4. OpenWrt 加入了 luci 界面, 安装后重新登录后台就能看见
5. 添加了自定义标记值功能, 仅 OpenWrt 有效
6. 修复了一些神秘 bug
7. 修改了教程内容
8. 有点头晕

### 2026.5.18 (版本 v2.0.2-r4)

1. 修复 Windows 服务无法退出的问题

### 2026.5.19 (版本 v2.0.3-r1)

1. 添加旧版本 OpenWRT LuCI 兼容, 目前最低能支持 15.05
2. LuCI 与主程序分离, 分成两个软件包

### 2026.5.22 (版本 v2.0.4-r2)

1. 重构 LuCI 代码, 更简洁
2. 添加在 LuCI 查看日志的功能, 可以自由切换 `/var/log/esurfing/logs` 里面的日志查看
3. 更好的 LuCI 提示框, 更好的使用体验
4. logger 现在在类 Unix 系统中会自动创建父目录
5. 旧 release 不会再删除上传的文件

### 2026.6.1 (版本 v2.0.4-r3)

1. 合并部分学校认证问题修复代码 (pr [传送门](https://github.com/BadGhost520/ESurfingClient-CVersion-Snapshot/pull/6))
2. 修复 OpenWRT LuCI 日志文件过大时无法读取日志问题
3. 桌面程序添加了 Web 服务器相关代码, 但未启用

### 2026.6.2

1. 将分支全部放到主仓库, 并重新命名分支名, 以 master 命名 v2 主分支, v1 命名 v1 旧分支
2. Github action 生成预发布版本时会删除旧预发布版本, 并发布最新预发布版本

### 2026.6.5

1. 做出了桌面端基础的 Web 页面框架
2. Github action 会自动打包替换 Web 文件

### 2026.6.6

1. 将 LuCI 读取日志的间隔调整为 5 秒读取一次
2. 桌面端 Web 页面进一步完善

### 2026.6.7

1. 桌面端 Web 页面框架搭建完成, 主程序取消注释启动 Web 服务器代码
2. 桌面端 Web 添加实时认证状态以及联网状态监测功能
3. 重试登录的时候如果正在关闭, 程序会在短时间内不断重试, 在开头添加了终止代码

### 2026.6.8 (v2.0.4-r5)

1. (00:35) 修复 Web 文件打包时错误的 sed 替换
2. (00:43) 因为总是漏掉 arm 版 macos 的工作流修改, 所以决定修改文件名前缀

### 2026.6.10 (v2.0.4-r6)

1. (02:03) 刚刚测试发现互联网已连接时的循环退不出去
2. (02:03) 现在桌面端的 Web 页面能够读取从后端传来的配置文件了

### 2026.8.31 (v2.0.5-r2)
1. (00:44) 因算法更迭, 故暂将 phone 通道关闭, 换为 pc 通道
2. (00:45) 算法目录结构优化
3. (00:48) 主程序目录 esurfingclient/esurfing 换为 esurfingclient/main
4. (00:51) 合并了 OpenWRT arm64 构建脚本和配置文件 pr, 目前能够构建通用 arm64 版本 ipk 包
5. (01:00) linux 版本段错误修复
6. (01:01) 喝了口绿茶, 其实鬼鬼的电脑重装过了xwx, 重新配环境配了好久
7. (01:27) 注释了不需要的算法, 并且将函数命名优化

### 2026.8.31
1. (09:52) 合并了新的网络检测方案 PR

### 2026.9.1 (v2.0.6-r1)
1. (02:04) 添加了新安卓算法
2. (02:05) 将 UA 显示日志换为 INFO 级别
3. (02:18) 小小优化一点结构

### 2026.9.4 (v2.0.6-r2)
1. (20:06) 合并 OpenWRT 配置加载修复 PR

### 2026.9.5 (时间控制功能)
1. 新增账号级 `time_windows` 时间控制字段, 支持按星期和跨天/跨周窗口
2. 冷启动时按系统本地时间判断是否在允许时段, 不在时段则不启动认证线程
3. 新增时间控制定时线程, 到开始时间自动启用账号, 到结束时间自动下线并禁用
4. 支持 OpenWRT 多账号以及桌面端配置透传
5. 非法的 `time_windows` 会拒绝启动程序

### 2026.9.11
1. (01:31) 写了依托网络检测函数, 而且还没写完

### 2026.9.11 (v2.0.7-r1)
1. (18:16) 新的网络检测函数

### 2026.9.11 (v2.0.7-r2)
1. (23:43) 更新 OpenWRT LuCI, 新增日志下载和日志自动置底功能
2. (23:44) 修改使用文档

### 2026.9.12
1. (00:22) 新增 Q&A
2. (01:01) 不赖
3. (13:47) 调整了一下代码结构

### 2026.9.13 (v2.0.8-r1)
1. (03:01) 移除了 OpenSSL 依赖, 并且统一了架构命名！！！现在二进制文件的大小只有 200kB 出头了！！！
2. (03:01) 好饿
3. (03:21) 更新了教程文档
4. (04:09) 更新了好多文档

### 2026.9.13 (v2.0.8-r2)
1. (19:32) 将所有 OpenWRT 主程序插件包工作流合并至一个工作流
2. (19:34) 添加了 OpenWRT 所有架构的适配包
3. (20:12) 给文档添加了 OpenWRT 架构自查表
4. (20:38) 给总工作流添加了所有的架构编译表
5. (21:56) 合并了 OpenWRT LuCI 工作流

### 2026.9.14
1. (22:31) 完善了桌面端 Web 页面
2. (22:31) 将 VERBOSE 级别日志的登录 xml 取消回显, 以消除敏感内容输出
3. (22:38) 桌面端文件打包时会附带默认配置文件

### 2026.9.15 (v2.0.9-r1)
1. (01:32) 验证了桌面端 Web 页面
2. (01:33) 何移位
3. (01:34) 合并了 Android 端代码
4. (02:04) 给桌面端 Web 文件改一下名

### 2026.9.15 (v2.0.9-r2)
1. (19:35) 给网络检测加上了雷霆的五个轮询网址
2. (19:35) 现在 curl 超时时间可以在配置文件修改了
3. (19:35) 现在重试时间会扣除 5 倍的总超时时间, 如果不够扣就不扣
4. (19:37) 什么俺寻思写法

### 2026.9.15 (v2.0.9-r3)
1. (22:53) 现在只剩四个轮询网址了
2. (22:53) 添加了一个 CURL 错误代码解析
3. (22:54) 预发布 Release 也有 body 了
4. (22:55) 尝试修复网络检测算法的问题
5. (22:58) 生成的默认文件同步

### 2026.9.17 (v2.1.0-r3)
1. (21:08) 程序改为安装成服务时使用多进程模式, 直接运行时仍旧按以前的多线程流程运行
2. (21:11) 另外再写

### 2026.9.18
1. (08:30) curl 错误等级调整为 ERROR 和 INFO 可显示的等级
2. (08:37) 新增 `-r` / `--role` 与 `-a` / `--account` 参数, 认证与 Web 各自成为独立进程
3. (08:37) 新增 `--list-accounts` / `--control-port` / `--control-token` / `--web-listen` 参数
4. (08:37) 认证与 Web 之间用一条只监听回环的控制通道通信, 带一次性令牌
5. (08:37) 关闭时先停 Web 再停认证, 顺序固定下来
6. (08:37) 桌面端装出来的服务改为先拉起一个守护进程, 由它看住认证与 Web
7. (08:37) 守护进程按指数退避重新拉起退出的子进程 (1 秒到 60 秒), 存活满 60 秒算健康
8. (08:37) 守护进程被强杀时子进程会跟着退出, 不再留下孤儿进程
9. (08:37) OpenWrt 改为每个账号起一个独立进程, 单个账号崩溃不影响其它账号
10. (08:37) 新增看门狗, 进程卡死时会自己退出, 交给 procd / systemd / 服务管理器重新拉起
11. (08:37) 修复关闭时不会登出的问题 (信号处理里直接退出, 收尾流程跑不到)
12. (08:37) 新增会话存档, 被强杀或断电后下次启动会先补一次登出, 不用干等服务器踢下线
13. (08:37) 修复 Windows 上停止服务时子进程被强杀、来不及登出的问题
14. (08:37) 日志改为一次 write 落盘, 多个进程共写同一份 run.log 不再互相穿插
15. (08:37) 轮转文件名加上进程号与序号, 避免同一秒内多次轮转互相覆盖
16. (08:37) 日志标识改为 `[auth#1 pid=1234] [main]`, 一眼看出是哪个进程的哪个线程写的
17. (08:37) OpenWrt 每次启动会先把上一轮的 run.log 归档成 `<时间戳>.log`
18. (08:37) 桌面端 Web 默认只监听 127.0.0.1, 原来监听 0.0.0.0 而接口无鉴权、会返回明文密码
19. (08:37) Web 启动日志的文案由「后台访问地址」改为「访问地址」, 内容为实际监听地址
20. (08:37) 桌面端 Web 页面区分「未认证」与「认证进程没在跑」, 不再把后者显示成前者
21. (08:37) 修复配置文件缺失挂起时按 Ctrl+C 无响应、日志不落盘的问题
22. (08:37) 修复守护进程遇到配置问题时挂住不退出 (看起来服务在跑, 实际什么都不干)
23. (08:37) 单进程模式的行为一行未变, 有与旧版本逐行比对日志的回归测试兜着
24. (08:37) 修复重启进程时参数丢失, 重启后角色会变成另一个
25. (08:37) 修复认证进程把网络检测排在时间窗口判断之前, 非允许时段仍会去轮询网络
26. (08:37) 修复 Windows 上枚举成员与 `<windows.h>` 的宏撞名导致编译不过
27. (08:37) 修复 Windows 上把句柄强转成整数, 日志里的 PID 在高位丢失
28. (08:37) 修复 macOS 分支缺少 include (那个分支从来没被编译过)
29. (08:37) 修复账号列举在桌面分支段错误
30. (08:37) 修复 OpenWrt 变体出现未使用函数警告
31. (08:37) 修复 init.d 的启动提示跨多行
32. (10:19) 修复网页界面路径问题

### 2026.9.19 (v2.1.0-r5)
1. (12:58) 观察到每次都是 `http://connect.rom.miui.com/generate_204` 发生 DNS 解析错误, 将其剔除

### 2026.9.19 (v2.1.1-r1)
1. (18:30) 配置文件新增三个参数: `web_external_acc` (Web 服务是否允许外部访问, 默认关闭), `log_dir` (日志基目录, 默认 `"./"` 即程序所在目录), `web_port` (Web 服务端口, 默认 8888)
2. (18:30) 日志目录改为可配置: `log_dir` 是【基目录】, 日志放在它下面的 `logs` 里 (与 OpenWrt 上 `/var/log/esurfing/logs` 的形状一致); 默认 `"./"` 即程序所在目录, 于是桌面端的日志仍旧在 `<程序目录>/logs/run.log`; 相对路径按程序所在目录解析 (程序装到 /usr/local/bin 或注册成服务时工作目录是 / 或 System32, 按工作目录解析会把日志写到谁也想不到的地方)
3. (18:30) OpenWrt 上基目录写死 `/var/log/esurfing` (日志即 `/var/log/esurfing/logs`, init.d 与 LuCI 的日志页都按这个路径取日志), 配置里写了别的基目录会在日志里提示一句"不生效"
4. (18:30) Web 服务的监听地址改由配置决定: `web_external_acc` 关闭时只监听 127.0.0.1, 打开时监听 0.0.0.0, 端口取 `web_port`; 打开外部访问时会额外提醒一句 (接口没有鉴权, 还会返回明文账号密码)
5. (18:30) 移除命令行参数 `--web-listen`, 端口改到配置文件里改; 守护进程也不再向子进程转发监听地址
6. (18:30) Web 端口与外部访问开关是启动时绑上去的, 改完要重启程序 (或重启服务) 才会生效
7. (18:30) 单进程模式改为先加载配置再启动 Web 服务: 端口要在绑端口之前就位, 顺序反过来就只能按默认端口开出去
8. (18:30) 桌面端网页界面 (设置页) 新增"日志目录 / Web 服务端口 / 允许外部访问"三项; 保存时若改的是端口或外部访问开关, 会提示需要重启才生效
9. (18:30) LuCI 界面同步: 这三项在 OpenWrt 上不生效 (日志目录写死, 也没有 Web 服务), 界面上不提供, 但复位写出的配置里补上了这三个字段, 日志页补了一句日志目录的说明
10. (18:30) 日志接口只认 `.log` 结尾的文件, 不再把目录里的其它文件也当成日志列出来、读出来 (基目录由配置决定, 指到某个已有目录上时, 不加这个判断就会把里面的无关文件一起列出来)
11. (18:30) 日志目录在配置加载之后才定下来, 而配置加载之前就已经在写日志了, 因此换目录时会把已经写下的那几行一起搬过去, 免得启动日志留在默认目录里没人收尾 (搬动只在目标还不存在时进行, 否则会覆盖别的进程的 run.log; 日志目录与程序不在同一个盘时搬不过去, 那几行会留在原目录)
12. (18:30) 默认的日志位置: Windows 与旧版一致 (程序所在目录下的 `logs`), Linux / macOS 从原来的 `/var/log/esurfing/logs` 改成了程序所在目录下的 `logs` —— 想保持原样就把 `log_dir` 写成 `/var/log/esurfing`, 它下面的 `logs` 由程序自己建; 在配置里改完之后要重启程序才会换到新目录
13. (18:30) 桌面端网页界面的"日志目录"一项填的是基目录 (日志会进它下面的 logs), 说明文字里写清了这一点; `doc/Desktop.md` 与 `doc/OpenWRT.md` 同步补上三个新参数
14. (19:20) 新增配置补全: 程序每次读取配置文件时会检查参数是否齐全, 缺的按默认值补上【并写回配置文件】, 日志里逐条说明补了什么 (只补缺的键, 已经写过的参数一律不动, 哪怕写的就是默认值)
15. (19:20) 账号里缺的字段也会补 (`channel` / `mark` / `time_windows` 等); `accounts` 整个缺了会补一份账号模板, 把账号密码填上就能跑, 不用再照文档抄一遍格式
16. (19:20) 配置文件只读 (或没有写权限) 时: 补全只作用在内存里, 程序照常按补全后的值运行, 日志里给出一条写回失败的告警
17. (19:20) 顺手把几处过时的默认值统一到 States.h 的宏上: 连接超时默认 7 秒、总操作超时默认 10 秒 (原来日志里写的是 3s / 5s, 与程序真正的默认值对不上), 网页界面里的兜底值也一并跟上
18. (22:30) `log_dir` 的路径识别补齐: 支持 UNC 路径 (`\\服务器\共享名\...`) 与扩展长度前缀 (`\\?\D:\...`、`\\?\UNC\...`) —— 之前这两种写法会因为 `\\` 与服务器名拿去 CreateDirectory 而失败, 最后默默退回默认目录
19. (22:30) `C:dir` 这种"盘符相对"写法不再当成绝对路径: 它指的是 C 盘当前目录, 而程序的工作目录随环境变 (服务模式下可能是 System32), 按它解析等于把日志写到谁也想不到的地方 —— 现在明确告警并退回默认目录
20. (22:30) `log_dir` 里写了环境变量 (`%TEMP%` / `$HOME` 之类) 会告警说明"程序不展开环境变量, 按普通目录名处理", 免得用户以为日志会落到临时目录里
21. (22:30) 路径结尾多带分隔符、正斜杠与反斜杠混用 (`D:/logs\\sub`) 都能正常处理; 路径里带 `..` 也是相对程序目录解析 (日志里显示的路径会带着 `..`)
22. (00:20) OpenWrt 上 `log_dir` 也生效了: 默认基目录是 `/var/log/esurfing`, 日志仍旧落在 `/var/log/esurfing/logs` (tmpfs, 重启即清); 想放到别的分区或 U 盘上就填绝对路径, 比如 `/tmp/esurfing` (日志在 `/tmp/esurfing/logs`); 相对路径按 `/var/log/esurfing` 解析 —— 那边的程序装在只读的 `/usr/bin` 里, 按程序目录解析没有意义
23. (00:20) 新增命令行查询 `--print-log-dir`: 按配置算出实际使用的日志目录并打印后退出; 日志目录由配置决定, 外部脚本不必自己去解析 JSON
24. (00:20) init 脚本的日志归档改为问程序要路径 (`--print-log-dir`, 读不出来时用默认值兜底), 因此改了 `log_dir` 之后归档也跟着走; 而且只在真正要用的时候才去问, `stop` / `status` 不会白跑一次程序
25. (00:20) LuCI 跟着配置走: 新版界面加了"日志目录"一项, 日志页按配置算目录; 旧版界面从配置里取 `log_dir` (默认 `/var/log/esurfing`) 再拼 `logs`
26. (00:20) 核对 GitHub 工作流: 没有任何工作流改配置文件的内容 (桌面/安卓的只是 `cp` 模板, OpenWrt 的只 sed 版本号; `openwrt-config/all.config` 只管 libcurl), 因此本次不需要动工作流 —— 另外确认 LuCI 工作流那两条 sed (版本号那一行) 没有被改坏

### 2026.9.20 (v2.1.1-r2)
1. (09:27) 使用 DNS 劫持的方法使 MIUI 检测使用固定 IP 访问, 彻底解决 DNS 解析错误问题

### 2026.9.20 (v2.1.1-r3)
1. (11:11) 将外部导入库统一放入 extern 目录
2. (11:12) 将日志轮转行数回调至 10000 行
3. (11:13) 7z-lzma, cJSON, mongoose 库升级到最新的版本
4. (12:26) 修复 OpenWRT 包启动时多一个日志的问题

### 2026.9.21
1. (14:47) 清理了多余的 include, 优化了代码结构
2. (14:49) 拆分 States 源文件

### 2026.9.22
1. (10:46) 拆分 NetClient
2. (10:47) 移动 NetClient 和 DialerClient 到 clients 目录

### 2026.9.22 (v2.1.2-r1)
1. (14:22) 包内自带 LICENSE, CMake 源文件清单收口, 修掉 LuCI 卸载时删错的路径
2. (14:24) 目录结构重构: 包目录自包含 (files/ 镜像目标 rootfs), 配置与图片分别归到 ci/ 与 docs/, 新增仓库级 .gitignore 并停止跟踪 .idea
3. (14:24) 构建逻辑从工作流收进仓库: scripts/build-portal.sh (网页资源构建)、scripts/sync-version.sh (版本号统一从 CMakeLists 分发)、ci/toolchains/mingw64.cmake
4. (15:28) 拆分 PlatformUtils.c (1708 行) 为 12 个按职责划分的文件
5. (15:46) 清理脚本与工作流的说明性注释, 容易踩的约定集中写到 docs/Notes.md
6. (15:48) 清理内置网页界面与 init 脚本的注释
7. (16:01) 拆分 Supervisor / Control / DialerClient / WebServer / NetClient / Logger / SimEvp 七个大文件, 拆分后最大的源文件不到 600 行
8. (16:01) 按"只保留函数说明与函数内步骤注释"清理全仓库注释
9. (20:14) 拆分出来的源文件统一为 PascalCase 命名 (LogoutState 这种)
10. (21:12) 主程序目录 esurfingclient/main 换为 esurfingclient/app

### 2026.9.22
1. (23:44) 尝试为 OpenWRT apk 包添加签名

### 2026.9.23
1. (02:24) 为 OpenWRT apk 包签名成功
2. (02:24) 去除 esurfingclient/Makefile 中多余的重启
3. (02:38) 日志归档改为"运行结束时"进行 (停止时先等实例退出再改名), 不再等到下次重启时才改名; 启动时只做兜底
4. (02:42) 卸载时清理残留文件: 会话存档 `.logout`、默认日志目录、升级时留下的模板文件

### 2026.9.24 (v2.1.3-r1)
1. (01:55) 修复了 iOS 和 MacOS 通道无法初始化加解密工厂的 bug
2. (01:55) 修复了偶现网络已连接的日志大量刷出的 bug

### 2026.9.24 (v2.1.3-r2)
1. (12:23) 尝试修复升级 OpenWRT 包时仍会执行 postrm 命令的问题

### 2026.9.24 (v2.1.3-r3)
1. (12:44) 合并 loongarch64 linux 构建 PR
2. (15:28) 修复 loongarch64 linux 构建脚本, 适配新版本

### 2026.9.24 (v2.1.4-r5)
1. (17:28) 修复 base64 编解码函数 bug, 修复 iOS, MacOS 强杀后重启登出假成功
2. (17:29) 修复前台模式强杀重启不会登出的 bug

### 2026.9.29 (v2.1.5-r1)
1. (19:51) 合并 Windows 通道加解密算法
2. (19:51) 修改 LuCI 相关文本
3. (20:07) 优化 OpenWRT Makefile

### 2026.9.29
1. (23:13) 尝试使用新的 OpenWRT 编译工作流

### 2026.9.30
1. (01:26) 优化了编译工作流, 大大降低了 github 缓存使用量

### 2026.10.2 (v2.1.5-r3)
1. (02:41) 优化了 OpenWRT 包的安装逻辑, 升级时会自动重启服务, 并且不会出现神秘删除配置文件现象
2. (02:42) 合并 MacOS 编译工作流

### 2026.10.4 (v2.1.5-r4)
1. (15:20) 修复安装/升级 OpenWRT 包时"重复启动服务"的问题: 包自己的 postinst 不再重启服务, 升级时 opkg 的 prerm 停旧实例、postinst 起新实例已经各代劳一半, 自己再来一次等于装一次包启动两遍
2. (15:22) init 脚本不再自己 kill 认证实例 (procd 会把它当成实例意外退出, 5 秒后再拉起一个, 退出慢的时候就是多出来的一次重启), 停止交给 procd, 日志归档挪到 procd 回收完实例之后的 service_stopped 钩子里
3. (15:24) init 脚本给认证实例补上 term_timeout 15, 停止时给登出请求留够时间 (原来这份余量是脚本自己等出来的), 免得账号被 SIGKILL 掉挂在服务端在线
4. (15:58) 顺手把 docs/OpenWRT.md 里"启动时归档"的说法改成"停止/重启服务时归档", 跟现在的行为对齐
5. (16:12) 在 25.12.3 (apk 包管理器) 的实机上核对了 apk 的安装/升级流程: 升级只跑 post-upgrade 钩子, 不会先停服务, 也不会跑 post-deinstall (所以升级不会清配置文件与自启软链接)
6. (16:33) init 脚本新增 running_old_binary: apk 升级后老实例还挂在被换掉的旧程序上 (`/proc/<pid>/exe` 带 "(deleted)"), 而 procd 只在实例定义变了时才重启实例 —— 这时由脚本先停一次再起, 新版本才真的生效, 整个升级仍然只重启一次; opkg 那边 prerm 已经停干净, 这里是空操作

### 2026.10.4 (v2.1.5-r5)
1. (17:12) 修改了 init.d 脚本的输出内容

### 2026.10.5 (v2.1.5-r6)
1. (20:24) 新增了更多架构支持
2. (20:25) 优化合并了工作流

### 2026.10.15
1. 修复了前台运行 (不带任何参数) 时 Web 服务起不来的问题: 原来要等配置加载成功才起 Web 服务, 而配置不可用 (首次运行时默认配置里 `enabled` 是关的 / 账号没填全 / JSON 写坏 / 配置文件还没生成) 时程序会在加载阶段就挂住, 网页也跟着起不来 —— 网页恰恰是用户改配置的入口, 等于把人锁在门外
2. 现在 Web 服务在业务配置之前启动, 配置不可用时程序停在"等配置"状态: 网页照常提供, 不做认证; 用户在网页上改好配置点"保存并应用"就自动重新加载并接着跑, 不用重启程序 (端口还是按配置里的 `web_port` 来: `load_cfg` 把这两个参数的解析提到了所有"配置不可用"的判断之前)
3. 配置不可用时配置页显示的是配置文件里的内容 (而不是空白): 之前内存里的账号是空的, 用户一点保存就把自己写过的账号密码覆盖成空的了 —— 现在 `load_cfg` 改成"尽力解析", 能读出来的值 (账号 / 密码 / 通道 / 时间控制 / Web 端口) 都会照样填进内存, 只有"能不能用"由返回值决定
4. 服务模式 (监管进程) 同步: 配置不可用时只拉 Web 子进程并保持"等配置"状态 (以前监管进程直接退出, 只会被服务管理器反复重启), 配置改好后自动补上认证子进程; 运行中配置变得不可用时也不再反复拉起认证进程 —— 监管进程每 1 秒看一眼配置文件的修改时间与大小, 变了才重新解析 (解析会写日志, 不然就把日志刷满了)
5. 去掉了 `cfg_halt` 的挂起等待: 等配置这件事交给各主流程的"等配置"循环, 等的时候网页是活的, Ctrl+C / 停止服务照样能打断
6. 网页界面: "保存并应用"之后的提示改成"程序正在重新加载配置"(以前写的是"程序即将重启", 与现在的行为不符); `docs/Desktop.md` 补上"配置还没弄好也能先用网页填"的说明
7. 补掉第 2 点里的一个漏洞: 程序刚加载完配置、还在等网络进入"需要认证"的阶段时 (这时认证线程还没创建), 网页上的"保存并应用"没人接 —— 把 `enabled` 改回 false 之后程序照旧往下走。现在 `wait_need_auth` 会认"配置变了"并把控制权交回主循环 (新的 `WAIT_CFG_CHANGED`), 预认证阶段、线程守护、认证进程三处都会重新加载并按结果决定是继续还是回到"等配置"; 网页的"保存并应用"只要认证进程还在就放行, 不再要求"正在认证" (刚加载完配置、还没开始认证也是正常状态)

### 2026.10.15 (源码目录重构)
1. `src/` 顶层扁平化: `clients/dialer/` → `dialer/`, `clients/net/` → `net/`, `states/` → `core/`; `clients/` 与 `states/` 这两层目录不再需要 (各模块本来就只有一两个子目录)
2. `utils/` 这个 20 个文件的杂货铺按用途细分: `log/`(日志系统 7 个) `platform/`(程序位置 / 父进程 / 网卡 4 个) `runtime/`(装服务、关闭钩子、看门狗 3 个) `time/`(时间与时间窗 2 个) `text/`(字符串 / 字节 / XML 3 个) `sim/`(自实现的 EVP 与线程 / 进程封装, 原地不动)
3. `LogoutState.c/.h` 从 `utils/` 挪到 `dialer/`: 它只服务认证会话 (会话存档 - 补登出), 引用它的三个文件本来就都在 dialer 里
4. `include/` 与 `src/` 同构, 头文件跟着源码一起搬
5. `PlatformUtils.h` 原来是大杂烩 (22 个函数分在 7 个 `.c` 里, 被 35 个文件引用), 按归属拆成 `utils/platform/PlatformUtils.h`(程序路径 / 父进程 / 网卡, 含 `SEP` 与平台系统头)、`utils/time/TimeUtils.h`(墙钟 / 单调钟 / 睡眠 / 格式化)、`utils/text/TextUtils.h`(字符串 / URL 参数 / base64 / 随机数 / 门户 XML); `print_log_dir` 并进 `utils/log/Logger.h`
6. 拆分顺带暴露并修掉了一批"靠 `PlatformUtils.h` 顺带包含"的隐式依赖 (平台系统头、`core/States.h`、`config/Config.h`、`inttypes.h` 等): 现在每个文件都直接包含自己用到的东西, 编译器 (`-Wimplicit-function-declaration` 在 GCC 14+ 是错误) 逐条盯着
7. `CMakeLists.txt` 的源文件清单与 `target_sources` 按新目录重排; `docs/Compile.md` 的目录说明、`docs/Notes.md` 里的源码路径引用一并更新, 并在 Notes 开头补了一节"目录怎么分"

### 2026.10.17 (v2.1.7-r2)
1. 尝试将 release 自动化

### 2026.10.17
1. 更新 README, 添加快速自查架构指令

### 2026.10.19 (Windows 辅助工具整理)
1. 仓库清洁: Windows 网速/热点工具的 6 个开发期测试脚本 (`Test-NetworkPanel.ps1`、`Test-HotspotBridge.ps1`、`Test-ReleasePackage.ps1`、`test-traffic.cjs`、`test-hotspot.cjs`、`test-packaged-page.cjs`) 从 `tools/windows-network-panel/` 移出, 放到不被 git 跟踪的 `TEMP/tools-tests/` (连同说明), 仓库里只留运行必需的文件
2. 打包脚本 `scripts/package-windows-helper.sh` 加了完整性校验: 复制完逐个核对发布包里的工具文件是否齐全、目录里有没有混进多余文件 (比如测试脚本), 缺一个或混进一个都直接让打包失败, 不再等用户双击才发现包不完整
3. 修掉一个只在 CI 上出现的隐患: `.cmd` 的仓库 blob 是 LF, 而 CI 在 Linux 上检出后直接打包, 用户拿到的批处理可能只有 LF —— 现在 `.gitattributes` 把 `*.cmd` / `*.ps1` 钉成 `eol=crlf`, 打包脚本复制完还会再把 `.cmd` 归一成 CRLF
4. `tools/windows-network-panel/README.md` 随之调整: 去掉用户既不需要也拿不到的"开发验证命令", 换成随包可用的只读自查 (`Start-Dashboard.cmd -Check`、`NetworkPanel.ps1 -Check`) 与兼容性实测入口 (`HARDWARE-VALIDATION.md` + `Get-ValidationEnvironment.ps1`, 这两个照旧随包发布)

### 2026.10.20 (辅助工具启动器修复)
1. 修掉 `Start-Dashboard.ps1` 自动定位客户端失败的问题: 原来只认发布包布局 (`<包根>\tools\windows-network-panel\`), 在本地构建目录 (`<构建目录>\windows-network-panel\`, 也就是 CMake 的 `CopyWNP` 目标复制的位置) 或把工具目录直接拷到客户端旁边时一律报 "Pass one explicit client path…", 用户只能手工传路径。现在依次尝试两种布局 (`..\..` 发布包 / `..` 客户端旁边), 都找不到才报错
2. 报错信息改成中文并说清"找过哪些路径、共找到几个 exe", 同时给出显式传参的例子; 本文件其它用户可见提示也一并改成中文。脚本写成 UTF-8 BOM (与 `NetworkPanel.ps1` / `TrafficCollector.ps1` / `HotspotBridge.ps1` 一致 —— PowerShell 5.1 在没有 BOM 时按 ANSI 解码, 中文会成乱码)
3. `tools/windows-network-panel/README.md` 补一句布局说明: 发布包布局与"客户端旁边的布局"都能自动定位, 本地 CMake 构建后直接双击 `<构建目录>\windows-network-panel\Start-Dashboard.cmd` 即可

### 2026.10.21 (辅助工具: 由主程序托管)
1. `TrafficCollector.ps1` 新增 `-StopFile <路径>`: 主程序关闭前先创建这个信号文件, 采集循环看到它就收尾退出 (热点桥接、临时文件、互斥锁照旧由 `finally` 清理, 实测从建文件到进程退出约 0.4 秒); 不传这个参数时行为完全不变 (仍靠"客户端进程是否还在"判断)
2. 采集脚本与启动脚本补上生命周期日志, 供主程序逐行转发进自己的日志文件: `[dashboard] 启动采集` / `采集已启动` / `热点桥接已启动` / `热点控制未启用: 原因` / `采集结束: 原因` (达到采样次数 / 收到主程序的停止请求 / 客户端进程已退出) / `已有另一个采集实例在运行`; 统一走 `[Console]::Out` 并逐行 flush (`Write-Host` / `Write-Warning` 在无控制台宿主里不可靠)
3. 输出被重定向时 (主程序接管) 自动切 UTF-8, 否则中文到了日志里会变成问号; 直接在控制台里跑时保持控制台编码, 避免不支持 UTF-8 的终端显示乱码
4. `Start-Dashboard.ps1` 新增 `-StopFile` 并透传给采集脚本, `-Check` 的回显里也带上它; `NetworkPanel.ps1` / `Start-NetworkPanel.cmd` 这些面板脚本未改动

### 2026.10.21 (辅助工具: 主程序侧接入)
1. 新增 `webserver/DashboardHelper.c` / `include/webserver/DashboardHelper.h`: Windows 下由主程序把随包的 `Start-Dashboard.ps1` 作为后台子进程拉起 (`powershell.exe -NoProfile -ExecutionPolicy Bypass -File … -ClientPath <自身> -WebPort <实际端口> -BridgePort 18889 -StopFile <日志目录>\dashboard-<pid>.stop`), 用 `CREATE_NO_WINDOW` 不弹窗; 脚本不在包里 (不是发布包/构建目录布局)、没有 `portal/assets`、非 Windows 时只记一条日志就跳过, 不影响认证主流程
2. 脚本的 stdout/stderr 走匿名管道, 由一条转发线程按行写进本程序的日志 —— 与主程序共用同一份 `run.log`, 行首仍带脚本自己的 `[dashboard]` 前缀; 路径一律用 W 版 Win32 接口 (目录名可能带中文)
3. 关闭顺序: `shut()` 里**先**停脚本再关 Web 服务与其它资源 —— 写停止信号文件让脚本自己收尾, 等 3 秒, 超时强杀; 之后才 `clean_logger()`, 保证脚本最后几行也落在这份日志里
4. 接入位置: 单进程模式 (`work()`) 与服务模式的 Web 子进程 (`work_web()`) 在 Web 服务起来之后拉起, `shut()` 统一停; 监管进程/认证进程不碰它
5. 实测 (Windows 构建 + 真实运行): 单进程 16/16 断言通过 (脚本被拉起、参数正确、`traffic.json` 正常、16 行 `[dashboard]` 输出进了 `run.log`、关闭时"关闭 Dashboard 辅助脚本 → 脚本采集结束 → 已退出 → 关闭 Web 服务器 → 退出程序"顺序正确、无残留 `.stop`); 服务模式 Web 子进程同样验证通过 (脚本先停, Web 后停)
6. 补充: 直接关闭控制台窗口时 Windows 会以 `STATUS_CONTROL_C_EXIT` 直接杀掉进程, 收尾流程来不及跑 —— 这种硬杀场景下脚本会在 1 秒内发现客户端不在了自行退出 (已实测), 不会留下孤儿进程

### 2026.10.21 (内置网速与热点接口, 仅 Windows)
1. 实时网速搬进主程序: 新增 `net/NetTraffic.c`, 每秒用 `GetAdaptersAddresses` + `GetIfEntry2` 采一次累计字节数, 与上一帧做差得到 B/s; 网页改从新增的 `GET /api/traffic` 取数据, 不再读 `portal/assets/traffic.json`, 也不再需要外部采集脚本。网卡枚举复用了 `utils/platform/NetAdapters.c` (新增 `get_adapters_stats()`), 只列"已连接且有网关"的网卡, VMware/Hyper-V 之类的虚拟网卡不再出现在下拉框里
2. 热点开关收进主程序: 新增 `GET /api/hotspot/status` 与 `POST /api/hotspot/start|stop`。开关动作异步执行 (接口立刻回 202, 页面轮询状态), 真正干活的是随包的 `HotspotControl.ps1` —— 每次动作只跑一次 WinRT, 常驻桥接进程、回环端口、令牌、来源白名单全部删掉, 页面改成同源请求
3. 热点状态由后台线程每 5 秒刷新一次并缓存, 且只在"网页最近 20 秒内来问过"时才起 PowerShell; Web 请求只读缓存, 不会把 Web 线程卡住
4. 新增通用件 `utils/runtime/ChildRun.c`: 跑一次子进程并把 stdout/stderr 收回来 (带超时强杀), 热点接口用它执行 PowerShell
5. 生命周期: `work()` / `work_web()` 启动采样与热点线程, `shut()` 先停它们 (热点 → 采样) 再收尾; 删掉 `webserver/DashboardHelper.c` 及"拉起并托管脚本"的那套调用
6. 页面: `traffic.js` 改请求 `/api/traffic`, `hotspot.js` 重写为同源 `/api/hotspot/*` (去掉桥接地址校验与令牌, 轮询从 2 秒放宽到 5 秒)
7. 顺带修掉两个真问题: `NetClientCurl.c` 里 `snprintf(out, out_len + 5, …)` 的界限比缓冲还大 (保护失效), `Config.c` 读配置忽略 `fread` 返回值 (短读时会把未初始化内容当 JSON 解析)
8. 实测 (Windows 构建 + 真实运行, 16/16): `/api/traffic` 返回 1 块真实网卡并逐秒更新速率; `/api/hotspot/status` 返回 `state=Off / capability=Enabled`、缺 `adapter` 参数回 400、未知路径回 404; 关闭后日志有"网速采样已启动/热点控制已就绪/网速采样已停止"、进程退出码 0、无残留 `HotspotControl.ps1` 进程
9. 其它平台 (Linux/macOS) 暂为桩: `/api/traffic` 返回 503、网页显示"网速采集不可用"; `get_adapters_stats()` 的非 Windows 分支留了位置与注释

### 2026.10.21 (热点脚本内嵌)
1. `HotspotControl.ps1` 不再作为文件随包发布: 脚本内容直接编进程序 (`HotspotControl.c` 里的 `HOTSPOT_SCRIPT`), 调用时用 `-EncodedCommand` 把"变量前缀 + 脚本"整段交给 Windows PowerShell —— 不再依赖发布包里的 .ps1, 也不会因为文件缺失或被删导致热点按钮失效
2. 顺带修掉三处真问题: ① PowerShell 的进度流会以 CLIXML 写到 stderr, 与 stdout 混在一起会破坏 JSON 解析 (现在只收 stdout, 脚本里也关掉了 `$ProgressPreference`); ② `ChildRun.c` 的命令行缓冲只有 `PATH_MAX * 4` 个宽字符, 装不下十几 KB 的 `-EncodedCommand`, 转换失败还会静默退化成"没带参数" (现在按 CreateProcess 上限放大, 失败会记日志); ③ 部分机器上网卡的连接名是空的, 热点列表改用描述兜底
3. 内嵌脚本的每行字面量都必须带 `\n`: C 的相邻字符串拼接不会自动换行, 漏掉会把整段脚本压成一行 (`#` 注释会把后面全吞掉) —— 生成时踩过这个坑
4. 验证: 15/15 通过 (网速速率逐秒更新、`/api/hotspot/status` 拿到 `state=Off / capability=Enabled`、参数与路径校验 400/404、关闭后日志完整且无残留 PowerShell 进程)

### 2026.10.21 (网卡列表: 未连接的也显示)
1. `/api/traffic` 的网卡过滤改成"只按是不是物理网卡筛": 有线 / Wi-Fi / 蜂窝都列, **未连接的也列出来** (每项多一个 `connected` 字段) —— 之前只列"已连接且有网关"的, 笔记本 Wi-Fi 没连时整块网卡消失, 看起来像没识别到
2. 虚拟网卡仍然排除, 判据换成 `MIB_IF_ROW2.InterfaceAndOperStatusFlags.HardwareInterface`: VMware / Hyper-V / 蓝牙个人区域网都是软件网卡, 不用再靠"有没有网关"猜
3. 页面: 下拉框里未连接的显示成"WLAN（未连接）", 选中未连接网卡时速度显示"—"并提示"所选网卡未连接，接上后自动开始统计"
4. 默认出口 (`defaultId`) 仍按 `GetBestInterface` 取, 只会是已连接的那块
5. 实测: 本机列表为"以太网 (connected=true, 有速率) + WLAN (connected=false, 0/0)", VMnet1/VMnet8 已排除; `/api/traffic` 等 15 项断言仍全过

### 2026.10.21 (Windows 工具目录清理)
1. 删掉已被内置实现取代的 4 个脚本: `TrafficCollector.ps1` (采样搬进 C)、`HotspotBridge.ps1` (桥接已删, 热点脚本内嵌)、`Start-Dashboard.ps1` / `Start-Dashboard.cmd` (只为拉起采集存在)。工具目录只剩 5 个文件: `NetworkPanel.ps1` + `Start-NetworkPanel.cmd` (独立窗口面板)、`Get-ValidationEnvironment.ps1` + `HARDWARE-VALIDATION.md` (实机验证)、`README.md`
2. 打包脚本 `scripts/package-windows-helper.sh` 的清单同步收缩成这 5 个, 并且不再在包根生成 `Start-Dashboard.cmd` (已经没有采集脚本可拉); Windows 工作流的 zip 列表也去掉那一行, 免得 zip 因文件不存而报错
3. `Get-ValidationEnvironment.ps1` 的"启动器检查"换成 `NetworkPanel.ps1 -Check` 能力探测, 输出多了 `panelAdapterCount` / `panelHotspotState` / `panelHotspotCapability`; `-ClientPath` 保留但只做"文件在不在"的检查
4. `tools/windows-network-panel/README.md` 与 `HARDWARE-VALIDATION.md` 按现状重写: 首页卡片由客户端内置, 本目录只提供独立面板与实机验证; 命令统一带上 `-ExecutionPolicy Bypass` (默认 Restricted 策略会直接拒绝执行脚本)
5. 实测: 打包脚本产出的工具目录正好 5 个文件、包根没有多余启动器; `NetworkPanel.ps1 -Check` 正常 (Adapters=1 / State=Off / Capability=Enabled); `Get-ValidationEnvironment.ps1` 输出 `panelCheck: Passed`; 构建目录里的工具副本也随之只剩这 5 个文件

### 2026.10.21 (删除 tools/windows-network-panel)
1. 整个 `tools/windows-network-panel/` 目录删掉 (`NetworkPanel.ps1`、`Start-NetworkPanel.cmd`、`Get-ValidationEnvironment.ps1`、`HARDWARE-VALIDATION.md`、`README.md`): 网速与热点都已内置在客户端 (网页上的两张卡片), 独立面板与实机验证清单不再需要
2. 连带清理: 删 `scripts/package-windows-helper.sh`; `CMakeLists.txt` 去掉把工具目录复制到构建目录的 `CopyWNP` 目标 (否则 `cmake -E copy_directory` 会因目录不存在让构建失败); Windows 工作流去掉"复制可选工具"这一步, zip 列表也去掉 `tools/windows-network-panel/` (发布包现在只有 exe + 配置 + `portal/`); 根 `README.md` 去掉指向该目录的链接
3. 客户端不依赖这些脚本: 实测用"只有 exe + ESurfingClient.json + portal/"的最小发布包运行 —— `/api/traffic` 正常 (2 块网卡)、`/api/hotspot/status` 正常 (`state=Off / capability=Enabled`)、首页可打开、退出码 0、无残留 PowerShell 进程, 7/7 断言通过
4. Windows 与 Linux 构建均通过 (Linux 侧重新 configure, 已无 `CopyWNP`); 构建目录里遗留的 `windows-network-panel` 副本也一并清掉了

### 2026.10.21 (Linux / OpenWrt 网速采样)
1. `get_adapters_stats()` 补上 Linux 实现: 用 `getifaddrs()` 枚举带地址的网卡, 累计字节数读 `/sys/class/net/<网卡>/statistics/{rx,tx}_bytes` (64 位, OpenWrt 的 busybox/musl 上也有), `connected` 看 `operstate` (up 或 unknown —— 后者是 PPP/PPPoE 这类不报载波的接口), 默认出口读 `/proc/net/route` 取 metric 最小的 0.0.0.0/0 作为 `defaultId`; 回环口按 `/sys/class/net/<网卡>/type == 772` 排除 (名字不一定叫 lo)
2. `NetTraffic.c` 的采样线程、锁、时钟、睡眠改成按平台分支 (Windows: `CRITICAL_SECTION` + `GetSystemTimeAsFileTime` + `Sleep`; POSIX: `pthread_mutex` + `clock_gettime` + `nanosleep`), 采样与 JSON 组装不再只编进 Windows —— Linux 与 OpenWrt 的网页也能看到网速卡片
3. `HotspotControl.c` 从 `NOT __OPENWRT__` 的源清单挪到主清单: `net_traffic_json()` 现在跨平台引用 `hotspot_control_available()`, 不挪 OpenWrt 链接会缺符号 (该文件在非 Windows 上就是返回 false 的桩)
4. 实测 (WSL Ubuntu, 与 OpenWrt 同一份代码路径): `/api/traffic` 返回 2 块网卡、`defaultId` 正确; 静默时 down/up 为 0, `ping -s 1400 -c 60` 期间采到 up ≈ 26.5 KB/s → 8.8 KB/s (与 60×1428 字节吻合); `/api/hotspot/status` 在 Linux 上照旧 501、`hotspotControls=false` (热点卡片不显示); 退出码 0, 日志有"网速采样已启动/已停止"
5. 其它: `-D__OPENWRT__=1` 配置构建链接通过; Windows 构建与最小发布包实测 (7/7) 无回归; macOS 仍是桩 (sysctl 待做)

### 2026.10.21 (Linux 热点控制)
1. Linux 上的热点开关走 NetworkManager (`nmcli`): 状态用 `nmcli -t -f DEVICE,TYPE,STATE device` 里的 wifi 设备, 能力看 `WIFI-PROPERTIES.AP`; 开关用 `nmcli connection up|down ESurfingHotspot`, 首次开启用 `nmcli device wifi hotspot ifname <网卡> con-name ESurfingHotspot ssid ESurfingHotspot` (NetworkManager 自己配 NAT 与随机密码)
2. `utils/runtime/ChildRun.c` 补上 POSIX 实现 (fork + execvp + 管道 + poll + 超时 SIGKILL), 与 Windows 共用同一套"跑一次子进程收输出"的接口
3. `HotspotControl.c` 重构成"平台无关骨架 + 每个平台两个原语" (`hotspot_query_status` / `hotspot_do_action` / `hotspot_platform_available`); 内嵌 PowerShell 脚本挪到独立文件 `webserver/HotspotScript.inc`; 顺带修两处: 状态刷新改成 200ms 心跳 (首个请求后约半秒就有真实状态, 而不是等 5 秒整拍), 网卡名在接口层就校验 (含非法字符直接 400, 不再先回 202 再在后台失败)
4. OpenWrt 走桩分支 (`__OPENWRT__` 下不探测 nmcli): 路由器上的热点另有配置入口, 页面不显示热点卡片 (`hotspotControls=false`)
5. 实测 (WSL, 用假的 `nmcli` 覆盖"没有无线网卡"的环境): `/api/traffic` 的 `hotspotControls=true`; `/api/hotspot/status` 返回 `wlan0 / Off / Enabled / connected=true`; `POST start` → 202 → 状态变 `On` 并提示 "SSID: ESurfingHotspot, 密码在系统的网络设置里查看或修改"; `POST stop` → `Off`; 非法网卡名 `wlan0;id` → 400 且 nmcli 调用记录里没有它; Windows (7/7 回归)、Linux、OpenWrt 三套构建均通过

### 2026.10.21 (macOS 网速采样与热点, 实验性)
1. macOS 网速采样: `get_adapters_stats()` 补 `__APPLE__` 分支 —— 用 `getifaddrs()` 的 AF_LINK 记录取 `en*` 等真实网卡的累计字节数与 `IFF_RUNNING`, 默认出口用 `sysctl(NET_RT_DUMP)` 解析路由表里目标全零的那条 (不走外部命令)。注意 macOS 的 `if_data` 是 32 位计数器, 4GB 会回绕一次 (按"计数器变小就重设基线"处理, 回绕那一帧显示"正在采样")
2. macOS 热点: 走系统的"互联网共享" —— 状态查 launchd 的 `com.apple.InternetSharing` job 有没有在跑, 开关用 `sudo -n launchctl load/unload -w /System/Library/LaunchDaemons/com.apple.InternetSharing.plist`; 没有免密 sudo 时不会卡住, 而是提示去「系统设置 → 通用 → 共享 → 互联网共享」手动操作
3. 实验性标记: `/api/traffic` 与 `/api/hotspot/status` 在 macOS 上多返回 `"experimental": true`, 网页上"实时网速""热点功能"两张卡片标题旁显示"实验性"角标; 因为手头没有 macOS 设备, 这两块**未在真机验证**
4. 编译验证: 用 zig 的 macOS 目标 (x86_64 与 aarch64) 对改动过的 4 个文件做 `-Wall -Wextra` 编译, 全部零警告 (期间修掉一个真错误: macOS 的 `rt_msghdr` 没有 `rtm_dst_len` 字段, 默认路由改成判断 RTA_DST 地址是否全零)
5. 回归: Windows (最小发布包 7/7)、Linux、OpenWrt 构建均通过; 页面 JS 语法检查通过

### 2026.10.21 (认证失败按服务端错误码分类处理)
1. 码表 `dialer/ErrorCode.c` (48 条, 两个码同时出现以 Sub-Error 为准) 分三类: 临时故障按 `{1,3,5,10,20,30}` 分钟退避, 可自愈错误 30 分钟慢重试, 需人工处理的停到重启或退出
2. `NetClientCurl.c` 的 `header_cb` 抓 `Error-Code` / `Sub-Error` (`post()` 补上 `CURLOPT_HEADERFUNCTION`, 值裁掉首尾空白); `error_code_record()` 写进 `runtime_status.last_error_*`
3. 界面: `/api/status/auth` 新增 `errorCode` / `subError` / `errorMessage` / `errorKind` / `errorAt`, 网页"连接状态"加一行"失败原因"
4. 验证: `TEMP/test-errorcode.c` 23/23; 三平台构建 + Windows 回归 7/7

### 2026.10.21 (OpenWrt 去掉网速采样与热点管理)
1. `NetTraffic.c` / `HotspotControl.c` / `ChildRun.c` 挪进 `NOT __OPENWRT__` 清单, `Shutdown.c` 的调用与 include、`get_adapters_stats()` 及其类型一并加 `#ifndef __OPENWRT__`
2. Linux **保持原样** (sysfs 计数器 + `/proc/net/route`), 网页网速卡片照旧可用
3. 验证: OpenWrt 链接列表已无这三个文件; Linux 运行 `/api/traffic` 正常; Windows 回归 7/7

### 2026.10.21 (全仓库代码整理)
1. 修三个隐患: 日志结构体依赖 `PATH_MAX` 导致跨编译单元尺寸不同 (改用自有 `LOG_PATH_MAX`)、`create_xml_payload()` 返回静态缓冲 (改 malloc, 四个调用点释放)、`logFn()` 越界写 (补边界检查)
2. 合并重复: `curl_resp_free` (14 处)、`dialer_request_plain` (四份五步骨架)、`cfg_read_*` (6 段字段样板)、`app_bootstrap` (4 个角色入口)、`fill_account` (两份账号循环)、`sim_thread_create_retry` (4 处)、`reply_json` (4 处)、`str_ieq`/`str_iprefix`、信道映射表化、`hotspot_base64` 合一
3. 命名: `()`→`(void)` 116 处、跨 TU `s_*`→`g_*` 208 处、include 分组 7 个文件、`LogLevel`→`log_level_t`、`clean_CDATA`→`clean_cdata`; 注释按"留事实、删叙事"压 9 个文件
4. 验证: 三平台构建 + 单测 23/23 + Windows 最小发布包回归 7/7

### 2026.10.21 (修复: 关闭控制台窗口后进程不退出)
1. 现象: 关终端后程序仍在后台跑。原因: `console_handler` 对所有事件都 `return TRUE`, 对 `CTRL_CLOSE` 等于告诉系统"我自己处理了", 系统就不再结束进程
2. 修法: 关窗口/注销/关机先置 `g_stop_requested` 让主循环收尾, 等 `g_shutdown_done` 最多 4.5 秒后 `return FALSE` 交回默认处理; `CTRL_C` / `CTRL_BREAK` 仍返回 TRUE
3. 验证: 三平台构建 + CTRL+BREAK 回归 7/7 (真实关窗口还需真机点一次 X 确认)

### 2026.10.21 (应用配置改为重启, 去掉认证热重载)
1. 单进程: 置 `g_need_restart` + `g_stop_requested` → `shut()` → `restart_process()` 重启自己, 新配置在启动时读取
2. 多进程: 控制命令 `apply_config` 让认证进程退出, 监管者用新配置重拉 (Web 与网页不受影响); 配置不可用的"等配置"状态仍走 `g_need_reload`
3. `g_cfg_loaded == false` 的三处检查降为兜底; 页面提示改为"正在重启"
4. 实测: 单进程 PID 30676→12428 (新 log_lv 生效); 多进程 auth 14036→30920、web 13632 不变

### 2026.10.21 (重新认证统一改为重启)
1. `request_restart_auth()` 与 `request_apply_config()` 同构; 控制命令 `restart_auth` 与 `apply_config` 合并到同一分支 (认证进程退出等监管者重拉)
2. `/api/restartAuth` 去掉"必须有线程正在认证"的 403; 页面按钮与弹窗文案改为"重启认证"
3. 实测: 单进程 13212→11752; 多进程 auth 31600→30176、web 14600 不变

### 2026.10.22 (修复: 点"重启认证"报 Cannot read properties of undefined)
1. 原因: `waitForRestart()` 属于 `Alpine.store('settings')`, 却被写成了 `store('config')`
2. 顺带去掉已失效的 403 分支, 并让 `waitForRestart()` 在多进程模式下 6 秒即刷新 (原来干等 30 秒)

### 2026.10.22 (重启后网页自动刷新)
1. 每个进程实例一个 `bootId` (`States.c: boot_id_get()`), `/api/status/sys` 与 `/api/status/auth` 都返回; 认证进程的值经控制通道 `status` 回复带出
2. 页面轮询到 `bootId` 变化即 `location.reload()` (通常 1~3 秒), 40 秒兜底刷新; 被动路径 (别处重启、崩溃后重拉) 也会刷新
3. 实测: 单进程 3def2137→95d69883; 多进程 authBoot 变而 sysBoot 不变

### 2026.10.22 (页面图标改用 assets/svg)
1. 21 处业务图标改为 `assets/svg/` 下的外部资源 (卡片、侧栏、页脚品牌), 标记只写 `--icon`, 由 CSS `.icon` 的 `mask: var(--icon)` 上色 —— 资源 fill 是固定深色, 直接 `<img>` 在深色主题下看不见
2. 保留 DaisyUI 主题切换的两个内嵌 svg, 以及按需求保持内嵌的页脚图标
3. 验证: 站点启动后 `/assets/svg/*.svg` 全部 200

### 2026.10.22 (portal 前端整理)
1. 请求层统一为 `httpJson(url, {method, json, timeout, signal})`, `request()` 只负责 `allow` 状态码; traffic / hotspot 不再各写一套 fetch + abort + 容错解析
2. 轮询统一 `startPolling(fn, ms, panel)`; 本地存储统一 `storage.get/set`
3. 配置字段表 `CONFIG_FIELDS {def, norm, out, check}` 驱动 `defaultConfigs` / `normalizeConfigs` / `buildPayload` / `validate`, 账号对象走 `normalizeAccount`
4. `formatBytes(value, perSecond)` 合并速率格式化, 错误提示统一 `describeError`, 多行 JSDoc 压成单行标注
5. 行数: main.js 1150→1070, index.html 777→748; `node --check` 通过, 页面与接口 200

### 2026.10.22 (C 侧去重: 命令行拼装与取自身路径)
1. `build_windows_cmdline()` 抽到 `ExecPath.c`, `SimProcess` 与 `SupervisorChild` 共用, 并补上超长时返回 false (原来静默截断丢参数)
2. `Service.c` 三个平台各自的 `GetExecutablePath()` 删除, 统一调用 `get_exec_path()` (490→478 行)
3. 实测: 单进程重启换 PID; 监管者拉起的 auth / web 子进程参数完整, apply 后只换认证进程

### 2026.10.23 (v2.1.13-r1)
1. 缝缝补补, 让 windows 通道认证得以正常

### 2026.10.23 (xtea 公共头整理)
1. `algo/xtea_common.h` 一直在用 (被 `mod_xtea.c` / `mod_xtea_iv.c` 以同目录裸包含引入, 与 `aes_common.h` / `sm4_common.h` 一致), 按全路径搜索会落空
2. 字节序工具下沉到 `CipherUtils`: 新增 `uint32_bswap()`, BE 读写改用已有的 `bytes_2_uint32_be()` / `uint32_2_bytes_be()` (原来头里的 `xtea_get_be32` / `xtea_set_be32` / `xtea_bswap32` 是重复实现)
3. 轮函数从"头文件里的 `static inline`"改成"`.h` 声明 + 新增 `algo/xtea_common.c` 实现", 与 `aes_common` / `sm4_common` 的结构一致; 这样头文件里没有函数体, IDE 开着头文件时不会再报 Unused function (原来还会因为 `mod_xtea_cbc_triple.c` 只用到其中一个函数而报其余几个)
4. `mod_xtea_cbc_triple.c` 不再包含该头 (它只需要 `uint32_bswap`); 它的轮函数**保持原样**: 实测换成标准 32 轮写法后密文变化而回环仍自洽, 说明是"sum 递减 + 预取密钥字"的变体, 已在函数头写明原因
5. 验证: `TEMP/test-xtea.c` 对三个 xtea 算法做密文基线与回环, 改造前后密文逐字节一致; 三平台构建 + Windows 回归 7/7; `algo/` 20 个工厂函数全部有引用、`src/` 下 81 个 .c 全部在 CMakeLists 中

### 2026.10.23 (Windows 算法表按非 fail 条目导入)
1. 按 `ticket_check_windows_*.tsv` 的探测结果, 只保留两台服务器上 **非 fail** 的条目: `121.8.177.212` 的 25 条新增 (key/iv 取自 `windows_key_iv.txt`), `14.146.227.141` 原有的 50 条里删掉 25 条被服务端拒绝的, 最终 windows 为 25 + 25 = 50 条
2. 新增 `KeyData.h` / `KeyData.c` 条目 25 组 (Type 2: 32B key + 16B iv; Type 5: 48B key; Type 6: 48B key + 8B iv), 对应算法 `create_aes_double_cbc_cipher` / `create_tea_triple_ecb_cipher` / `create_tea_triple_cbc_cipher`
3. 注释沿用上一轮格式: `// [算法] (Windows, [服务器 IP])`
4. 验证: `TEMP/test-win-algos.c` 对 50 个 windows 算法逐个做加解密回环, 50/50 通过; 分支总数 95 且无重复; 三平台构建 + Windows 回归 7/7

### 2026.10.23 (KeyData 注释格式与顺序整理)
1. `KeyData.c` / `KeyData.h` 的算法注释统一成 `// Algo Id: <ID> (<算法>) (<通道>)`, 通道取 Linux / Windows / Android / iOS / macOS (原来混着 "Android old)(已弃用)"、"Windows, cdy N"、以及 `// =====` 分组标题)
2. 条目顺序改成与 `create_cipher_factory()` 完全一致 (95 条), 两个文件顺序相同; 定义里混用的 `uint8_t` / `uint32_t` 类型保持原样
3. 说明: 9 条旧算法原来的"已弃用"标记在统一格式时去掉了, 需要的话可以加回 (例如写成 `(AES/CBC/NoPadding, 已弃用)`)
4. 验证: 三平台构建通过; 50 个 windows 算法回环测试 50/50; Windows 回归 7/7

### 2026.10.23 (桌面端统一为多进程)

1. 不带参数运行等同 `--role supervisor`
2. 删除桌面单进程路径, 仅 OpenWrt 保留
3. Web 层只走控制通道
4. 认证进程补时间控制线程
5. 控制通道加 `SO_REUSEADDR`, 客户端先关
6. Windows 补父进程死亡检测
7. 横幅只由入口角色打印

### 2026.10.24 (控制端口与网页接口)

1. 控制端口被占时自动顺延
2. 控制通道连接改非阻塞, 300ms 上限
3. `/api/status/auth` 合并成一次查询
4. 网速 / 热点接口不再被拖慢

### 2026.10.24 (联网检测)

1. 改为并发探测, 连接 3 秒 / 总 4 秒
2. DNS 与固定 IP 两路并行, 谁先通用谁
3. 一个域名可配多个固定 IP, 各占一路
4. 探测强制 IPv4, 避免 IPv6 白等
5. 全失败时按 30 秒限流打印结果码
6. `/api/status/online` 改读后台缓存
7. 修正"连接阶段超时"的判定
8. 补 `curl_global_init()`

### 2026.10.24 (日志等级)

1. 循环内每秒的状态日志降为 DEBUG
2. 线程级退出不再用 FATAL
3. 重试中的失败由 ERROR 降为 WARN
4. 协议字段与重试间隔降为 DEBUG
5. 应答发送失败、内存失败修正等级
6. 联网状态只在变化时提示一次
7. INFO 只留用户必须看见的内容
8. Web 访问地址改为单独提示
9. 修复关窗口时日志不归档
10. 未拿到认证地址时不再空转

### 2026.10.24 (控制台编码)

1. 控制台改用 UTF-8 代码页
2. 退出时恢复原代码页

### 2026.10.24 (认证地址学习)

1. 恢复从 302 学习认证页面地址
