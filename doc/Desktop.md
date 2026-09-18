# Windows, Linux, MacOS 环境使用教程

> [!NOTE]
> 教程版本: v2.1.0-r3

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

## 二、各个系统的执行方式 (前台运行)

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
> 网页文件在程序旁边的 portal 目录里, 别把它删了或者单独把主程序挪走
> 
> 默认只监听本机, 局域网里的其它设备访问不了; 确实需要的话加参数 `--web-listen 0.0.0.0:8888` (接口没有鉴权, 谨慎使用)

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
> 只是临时用一下的话可以不装服务, 按 `步骤二` 前台运行就够了, 行为完全一样

## 四、修改生成的 ESurfingClient.json

### 在程序所在目录能找到这个配置文件, 按照如下示例填写

```json
{
  "enabled": true,
  "log_lv": 4,
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

### JSON 参数详解

- enabled(布尔值): 程序是否启动
- log_lv(整形值, 有效范围 0-6): 日志等级, 等级越高日志显示内容越多, 数值为 0 时不输出任何日志
- accounts(数组): 账号数组
- username(字符串值): 账号
- password(字符串值): 密码
- channel(整形值, 有效范围 1-5): 认证通道
- mark(字符串值): 标记值 (高级功能)
- time_windows(字符串值): 时间控制, 可选, 数组; 每项格式 `{ "start": "mon 08:13", "end": "mon 23:57" }`, 支持跨天/跨周, 留空表示不限; 按系统本地时间判断

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
