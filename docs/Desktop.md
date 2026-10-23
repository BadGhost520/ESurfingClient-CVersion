# Windows, Linux, MacOS 环境使用教程

> [!NOTE]
> 教程版本: v2.1.1-r4

## v2 版本的使用十分简单, 跟着一步一步即可

## 一、使用前准备

### 1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 中下载相应的程序

### 2. 将程序放在自己想要的位置

> [!NOTE]
> macOS 的权限和隐私管理比较严格
> 
> Desktop 等目录不允许程序随意读取文件
> 
> 建议是把程序放进 `/usr/local/bin` 里面去使用

## 二、各个系统的执行方式 (直接运行)

> [!NOTE]
> 不带任何参数直接运行就是"守护模式", 也是 v2 唯一的前台运行方式: 程序会把自己拆成
> 三个进程 —— 一个监管进程看住另外两个, 一个认证进程负责登录, 一个网页进程负责提供网页。
> 三个进程的日志都写进 `logs/run.log`, 每行开头的 `[supervisor]` / `[auth#1]` / `[web]`
> 用来区分是谁写的。
>
> 关掉监管进程 (关窗口 / Ctrl+C / 任务管理器结束) 时, 两个子进程会跟着一起退出;
> 认证进程退出前会尽量把登出请求发出去。服务模式跑的也是这一套流程, 行为完全一致。

### 1. Windows 双击直接运行, 或者在终端执行如下指令

```shell
# Windows PowerShell or CMD (无特别权限要求)
.\ESurfingClient-*-windows-*.exe
```

### 2. Linux 在终端执行

```shell
# Linux Bash (需要 root 权限)
sudo ./ESurfingClient-*-linux-*
```

### 3. macOS 在终端执行

```shell
# macOS Zsh (需要 root 权限)
sudo ./ESurfingClient-*-darwin-*
```

> [!NOTE]
> 首次运行会在程序所在目录生成 ESurfingClient.json 配置文件和 logs 日志目录
> 
> 日志默认就放在这个 logs 目录里 (`logs/run.log`), 想换地方就改配置里的 `log_dir`
> 
> 注意是"程序所在目录", 而不是执行命令时所在的目录
> 
> 比如在 `C:\Users\bad_g` 下执行 `D:\Tools\ESurfingClient.exe`, 配置文件会生成在 `D:\Tools` 里面, 可以用 pwd 指令查看当前目录来对比
> 
> 所以 Windows 用双击执行是最方便的, 双击时程序目录和当前目录正好是同一个

> [!TIP]
> 程序自带网页界面, 默认地址 http://127.0.0.1:8888 , 运行起来之后用浏览器打开即可
> 
> 在上面能看实时认证状态, 也能直接填账号密码, 比手改 JSON 省事
> 
> 配置还没弄好时 (首次运行生成的默认配置里 `enabled` 是关的, 或者账号没填全) 程序也会先把网页起来:
> 这时它不做认证, 就停在网页上等你去改 —— 在网页上填好账号、打开开关, 点"保存并应用"就会自动生效, 不用重启程序
> 
> 运行中把它关掉 (`enabled` 改成 `false` 再"保存并应用") 也一样: 程序会停下来, 只留着网页, 想再开就打开开关再应用一次
> 
> 网页文件在程序旁边的 portal 目录里, 别把它删了或者单独把主程序挪走
> 
> 默认只监听本机, 局域网里的其它设备访问不了; 确实需要的话在配置文件的 `web_external_acc` 里开启 (接口没有鉴权, 谨慎使用)
> 
> 端口在配置文件的 `web_port` 里改, 默认 8888; 端口与外部访问开关都是启动时绑上去的, 改完要重启程序 (或重启服务) 才生效

## 三、各个系统的执行方式 (以自启服务的形式运行)

### 1. Windows 在终端执行

```shell
# Windows PowerShell or CMD
# 安装服务 (需要管理员权限)
.\ESurfingClient-*-windows-*.exe -i
# 卸载服务 (需要管理员权限)
.\ESurfingClient-*-windows-*.exe -u
# 帮助 (无权限要求)
.\ESurfingClient-*-windows-*.exe -h
```

### 2. Linux 在终端执行

> [!WARNING]
> 需要使用 systemd 管理服务的 Linux 系统才能使用

```shell
# Linux Bash
# 安装服务 (需要 root 权限)
sudo ./ESurfingClient-*-linux-* -i
# 卸载服务 (需要 root 权限)
sudo ./ESurfingClient-*-linux-* -u
# 帮助 (无权限要求)
sudo ./ESurfingClient-*-linux-* -h
```

### 3. macOS 在终端执行

```shell
# macOS Zsh
# 安装服务 (需要 root 权限)
sudo ./ESurfingClient-*-darwin-* -i
# 卸载服务 (需要 root 权限)
sudo ./ESurfingClient-*-darwin-* -u
# 帮助 (无权限要求)
sudo ./ESurfingClient-*-darwin-* -h
```

> [!NOTE]
> v2.1.0 起安装服务后跑的是多进程: 一个监管进程看住认证进程和网页进程
> 
> 任一子进程退出都会按退避自动重新拉起, 一个进程出问题不会牵连另一个
> 
> 所以服务管理器里看到的是监管进程, 停止服务时会把子进程一起有序关掉, 认证进程退出前会先登出
> 
> 只是临时用一下的话可以不装服务, 按 `步骤二` 直接运行就够了 —— 不装服务时跑的也是这一套多进程流程

## 四、修改生成的 ESurfingClient.json

### 在程序所在目录能找到这个配置文件, 按照如下示例填写

```json
{
  "enabled": true,
  "web_external_acc": false,
  "log_lv": 4,
  "log_dir": "./",
  "conn_timeout": 7,
  "op_timeout": 10,
  "web_port": 8888,
  "accounts": [
    {
      "username": "在这填账号",
      "password": "在这填密码",
      "channel": 3,
      "time_windows": []
    }
  ]
}
```

> [!NOTE]
> 别忘了改 `enabled` 参数
> 
> 懒得手改 JSON 的话, 直接在网页界面的"设置"页里填也一样: 程序会把网页先起来, 填完点"保存并应用"就会重启认证进程 (网页不受影响), 新配置随即生效
> 
> `log_dir` / `web_port` / `web_external_acc` 都有默认值, 不改也能用, 详见 `附 1`

## 五、重启程序 / 服务

> [!NOTE]
> 重启程序按照 `步骤二` 执行就可以
>
> 如果是安装了服务就需要往下看

### 1. Windows 可以在服务管理程序重启 `ESurfingClient Auth Service` 服务, 或者在终端执行如下指令

```shell
# Windows PowerShell
Restart-Service ESurfingClient
# Windows CMD
sc stop ESurfingClient
sc start ESurfingClient
# 查看状态
sc query ESurfingClient
```

### 2. Linux 在终端执行

```shell
# Linux Bash (需要 root 权限)
sudo systemctl restart esurfingclient
# 查看状态
sudo systemctl status esurfingclient
```

### 3. macOS 在终端执行

```shell
# macOS Zsh (需要 root 权限)
sudo launchctl bootout system /Library/LaunchDaemons/com.esurfingclient.auth.plist
sudo launchctl bootstrap system /Library/LaunchDaemons/com.esurfingclient.auth.plist
# 查看状态
sudo launchctl list | grep main
```

> [!TIP]
> 建议查看日志以确定程序运行情况

## 附 1: JSON 参数详解

- enabled(布尔值): 程序是否启动
- web_external_acc(布尔值): 网页服务是否允许外部访问, 默认 `false` (只监听 127.0.0.1); 改成 `true` 会监听 0.0.0.0, 局域网里的设备也能打开, 但接口没有鉴权、还会返回明文账号密码, 谨慎开启
- log_lv(整形值, 有效范围 0-6): 日志等级, 等级越高日志显示内容越多, 数值为 0 时不输出任何日志
- log_dir(字符串值): 日志的【基目录】, 日志放在它下面的 `logs` 里; 默认 `"./"` 即程序所在目录 (也就是日志在 `<程序目录>/logs` 下); 相对路径按程序所在目录解析, 也可以填绝对路径
- conn_timeout(整形值): 自定义 CURL 连接超时时长
- op_timeout(整形值): 自定义 CURL 总操作超时时长
- web_port(整形值, 有效范围 1-65535): 网页服务的端口, 默认 8888
- accounts(数组): 账号数组
- username(字符串值): 账号
- password(字符串值): 密码
- channel(整形值, 有效范围 1-5): 认证通道
- mark(字符串值): 标记值 (高级功能)
- time_windows(字符串值): 时间控制, 可选, 数组; 每项格式 `{ "start": "mon 08:13", "end": "mon 23:57" }`, 支持跨天/跨周, 留空表示不限; 按系统本地时间判断

> [!NOTE]
> 改完配置文件要重启程序 (或重启服务) 才会生效; 网页界面上的设置页也能改上面这些参数, 改完点"保存"写进配置, 账号之类的点"保存并应用"即可
> 
> `web_port` 与 `web_external_acc` 是启动时绑上去的, 只能重启生效
> 
> 漏写的参数不用怕: 程序每次读取配置时会检查一遍, 缺的按默认值补上并写回配置文件, 日志里也会说明补了什么
