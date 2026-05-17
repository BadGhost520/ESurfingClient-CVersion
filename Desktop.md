# Windows & Linux 环境使用教程 (版本 v2.0.1-r1)

> [!WARNING]
> 更新 v2 版本后此教程仅提供 v2 版本的教程
>
> 如若需要 v1 版本的教程, 可自行前往 v1 分支查看

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
> 运行之后会在用户所在目录生成一个 ESurfingClient.json 配置文件
> 
> 何为用户所在目录, 如下所示
> 
> 或者使用 pwd 指令查看

```shell
# Windows CMD
C:\Users\bad_g>
# Windows PowerShell
PS C:\Users\bad_g>
```
```shell
# Linux Bash
badghost@BadGhost:~$
```
```shell
# macOS Zsh
badghost@badghostdeMac ~ %
```

> [!NOTE]
> 所以 Windows 用双击执行是最方便的

## 三、各个系统的执行方式 (以自启服务的形式运行)

### 1. Windows 在终端执行

```shell
# Windows PowerShell or CMD
# 安装服务 (需要管理员权限)
.\ESurfingClient-*-windows-*.exe --install
# 卸载服务 (需要管理员权限)
.\ESurfingClient-*-windows-*.exe --uninstall
# 帮助 (无权限要求)
.\ESurfingClient-*-windows-*.exe --help
```

### 2. Linux 在终端执行

```shell
# Linux Bash
# 安装服务 (需要 root 权限)
sudo ./ESurfingClient-*-linux-* --install
# 卸载服务 (需要 root 权限)
sudo ./ESurfingClient-*-linux-* --uninstall
# 帮助 (无权限要求)
sudo ./ESurfingClient-*-linux-* --help
```

### 3. macOS 在终端执行

```shell
# macOS Zsh
# 安装服务 (需要 root 权限)
sudo ./ESurfingClient-*-darwin-* --install
# 卸载服务 (需要 root 权限)
sudo ./ESurfingClient-*-darwin-* --uninstall
# 帮助 (无权限要求)
sudo ./ESurfingClient-*-darwin-* --help
```

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
      "channel": "phone"
    }
  ]
}
```

> [!NOTE]
> 别忘了改 `enabled` 参数

### JSON 参数详解

- enabled: 程序是否启动
- log_lv: 日志等级, 1-6级, 等级越高日志显示内容越多
- accounts: 账号数组
- username: 账号
- password: 密码
- channel: 认证通道 (暂时没找到具体作用)
- mark: 标记值 (高级功能, 非 OpenWrt 系统无效)

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
sudo launchctl list | grep esurfing
```

> [!TIP]
> 建议查看日志以确定程序运行情况
