# Windows 网速与手机热点（可选工具）

双击 **Start-NetworkPanel.cmd**。需要 Windows 10/11、系统自带的 Windows PowerShell 5.1 和支持移动热点的无线网卡；无需安装依赖或重新编译 C 客户端。

1. 保持原来的校园网客户端正常运行。
2. 在面板选择校园网使用的上联网卡；默认选择 Windows 当前互联网网卡。下载、上传速度每秒更新，第一次采样显示“采样中”。统计所选网卡的全部流量（含共享流量），单位为 B/s、KiB/s、MiB/s，不是套餐带宽或测速结果。
3. 不需要热点时不用点击任何按钮。需要时点击“开启热点”，等待状态变为“已开启”，手机连接显示的热点名称。名称和密码在“设置热点名称 / 密码”中查看或修改；面板不保存密码。
4. 使用完点击“关闭热点”。面板启动和退出都保持系统原有热点状态；若原来已开启，不会替你关闭。没有自动开启、开机启动或后台重试。

热点调用 Windows 自带的移动热点接口。首次开启可能引发无线网卡重连，建议以太网作为校园网上联；无线硬件、驱动和学校策略可能限制共享，不能保证所有环境都无瞬断。切换过程中按钮禁用，完成后显示 Windows 返回结果；失败可打开系统设置查看。关闭窗口不会取消已提交给 Windows 的切换操作。

独立窗口不读取校园网账号配置，不启动/停止认证，不额外修改 DNS、路由、防火墙或网卡状态。窗口本身没有 HTTP 控制接口；下述可选网页热点桥接只监听本机回环。原认证程序无需替换。

## 首页仪表盘实时网速

首页新增“实时网速”卡片，沿用原有 DaisyUI / Alpine 样式和主题。采集进程每秒读取网卡统计，原子更新 `portal/assets/traffic.json`，页面从原端口读取；无新增网络监听、无认证进程重启、无热点操作。停采超过 6 秒显示过期，失去数据时不继续展示旧速度。

只有读取到辅助工具生成的数据后才显示网速卡片；热点卡片另外要求采集启用 `-HotspotControls`。没有部署辅助工具的平台保持原仪表盘，且不请求本机热点控制接口。停止采集后保留最后的数据文件，页面可显示已过期提示。

Windows 发布压缩包已包含辅助工具。完整解压后，双击根目录的 **Start-Dashboard.cmd**；只有一个 `ESurfingClient*.exe` 时自动定位，不需要手工复制脚本。原 EXE 仍可单独运行。

源码目录或存在多个客户端时，使用本目录的入口并传入实际客户端路径（不限定文件名或版本）：

```cmd
Start-Dashboard.cmd "C:\ESurfingClient\ESurfingClient.exe"
```

客户端已运行时只补充采集；未运行时启动原客户端。采集随客户端退出，重复启动通过互斥锁避免重复采集。入口同时启用可选的网页热点控制，启动时不会自动打开热点。

手工更新旧客户端时，使用发布包中已构建的 `portal` 文件；源码中的 index.html 仍有开发用 CDN 引用，不能直接当作离线发布页面复制。自行构建页面请使用 `scripts/build-portal.sh`。只启用网速而不启用网页热点控制时，可直接运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\TrafficCollector.ps1 -ClientPath "C:\ESurfingClient\ESurfingClient.exe"
```

只读采集依赖 Windows PowerShell 5.1；该卡片统计所选网卡全部流量，不是认证进程自身流量。网卡选择保存在浏览器，断开后需重新选择。仅启动原 EXE 而未启动采集时，认证可照常使用，网速卡片会提示启动采集。

## 首页热点卡片与顺序

仪表盘顺序为：连接状态、实时网速、热点功能、当前账号、程序信息。热点卡片支持选择共享网卡、查看状态与名称、手动开启/关闭；“名称 / 密码设置”打开 Windows 原生移动热点设置，网页不返回密码。

`Start-Dashboard.cmd` 使用 `-HotspotControls`，默认在 `http://127.0.0.1:18889` 启动桥接。来源只允许配置的页面端口，支持 `127.0.0.1` 与 `localhost` 两种地址；修改操作额外校验每次启动生成的令牌。省略此选项仍可单独采集网速。

页面使用自定义端口或桥接端口冲突时，可在压缩包根目录执行：

```cmd
Start-Dashboard.cmd -WebPort 9999 -BridgePort 19999
```

或从源码目录指定客户端：

```cmd
Start-Dashboard.cmd "C:\ESurfingClient\ESurfingClient.exe" -WebPort 9999 -BridgePort 19999
```

`WebPort` 必须与客户端已有的 Web 端口一致；启动工具不会修改客户端配置。此例允许 `http://127.0.0.1:9999` 和 `http://localhost:9999`，不允许其他端口或外部网站。页面从采集数据中读取桥接地址，只接受 HTTP 的 `127.0.0.1` 地址，不向外部地址发送热点请求。

开启/关闭请求异步调用 Windows WinRT；忙碌、不可用、服务失联时禁用相应按钮，失败显示原因，不自动重试开关。启动辅助进程、刷新网页和退出辅助进程均不会主动改变热点状态。不要将此服务绑定到外网。端口被占用或 Windows 不允许监听时网速采集继续，页面显示对应原因，不会连接占用该端口的其他进程。可换用 `BridgePort`，或使用系统设置/独立窗口；工具不自动提权或配置 URL ACL。

采集数据过期或读取失败时，热点控制会暂停并清除旧操作令牌，不使用过期状态发送开关请求。运行中的热点桥接出现监听异常时停止该桥接，网速采集继续；恢复需重新启动辅助程序，不自动重试热点开关。

验证覆盖只读真实状态、来源/令牌/方法拒绝、无效网卡、浏览器模拟开关与失败；真实热点切换和手机联网仍须用户按需点击验证。

## 开发验证命令

在本目录用 **Windows PowerShell 5.1** 执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Test-NetworkPanel.ps1
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File .\NetworkPanel.ps1 -Check
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Test-HotspotBridge.ps1
node .\test-traffic.cjs
node .\test-hotspot.cjs
```

第一条验证采样差值、计时、计数器重置和单位；第二条构造界面并只读采样网卡/热点能力，输出后退出。两条都不会开启或关闭热点。真实手机联网需由用户手动点击开启后验证。

其余检查覆盖来源与令牌校验、localhost/自定义端口、桥接地址限制和启动失败状态。压缩包入口支持 `Start-Dashboard.cmd -Check -WebPort 9999 -BridgePort 19999`，只检查路径和参数，不启动客户端或热点。

## 支持范围与限制

- 这是 Windows 10/11 的可选辅助工具，不提供 Linux、macOS、OpenWRT 的热点实现。云端验证覆盖 Windows x86、x64、ARM64 构建包；Windows 10、原生 32 位系统仍未实测。
- 用户已在一台设备上手动验证热点手机上网与电脑校园网正常；自动化页面测试仅模拟热点控制，不会切换热点。
- 不修改 C 认证流程，不需要替换已工作的客户端二进制文件。本次云端编译了 Windows x86、x64、ARM64 客户端；未覆盖全部平台/打包目标。
- Web 热点桥接可能需要 Windows URL 监听权限；权限不足或端口占用时保留网速统计，热点控制显示未连接。不自动提权、不改 URL ACL 或防火墙。

实现依据：[Windows 移动热点接口](https://learn.microsoft.com/en-us/uwp/api/windows.networking.networkoperators.networkoperatortetheringmanager)、[网卡统计接口](https://learn.microsoft.com/en-us/dotnet/api/system.net.networkinformation.networkinterface.getipstatistics)。
