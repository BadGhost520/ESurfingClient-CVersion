# OpenWRT 环境使用教程

> [!NOTE]
> 教程版本: v2.1.0-r4

## 一、从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 下载对应架构的 ipk 包, (可选)下载 LuCI 包 

## 二、上传到 OpenWRT 系统安装

> [!WARNING]
> 注意使用 apk 包管理器的 OpenWRT 系统 (25.12.0-rc1 及以上版本) 必须要在终端里用指令安装
> 
> 因为 OpenWRT 自带的软件包管理器安装默认是不带 `--allow-untrusted` 和 `--no-network` 参数的
> 
> 使用 opkg 软件包管理器的 OpenWRT 系统 (25.12.0-rc1 以下版本) 随意

```shell
# 终端安装
# opkg 包管理器 (OpenWRT 25.12.0-rc1 以下)
opkg install esurfingclient_*.ipk
# apk 包管理器 (OpenWRT 25.12.0-rc1 及以上)
apk add --allow-untrusted --no-network esurfingclient_*.apk
# 或者连同 LuCI 一起安装
opkg install esurfingclient_*.ipk luci-*-esurfingclient_*.ipk
apk add --allow-untrusted --no-network esurfingclient_*.apk luci-*-esurfingclient_*.apk
```

## 三、启动服务 (终端方式)

### 1. vi 修改 /etc/config/esurfingclient

```json
{
  "enabled": true,
  "web_external_acc": false,
  "log_lv": 4,
  "log_dir": "./",
  "conn_timeout": 7,
  "op_timeout":   10,
  "web_port": 8888,
  "accounts": [
    {
      "username": "账号",
      "password": "密码",
      "channel": 3,
      "mark": "",
      "time_windows": []
    }
  ]
}
```

> [!NOTE]
> `web_external_acc` / `web_port` 是桌面端的参数, OpenWrt 上不生效
> (那边没有网页服务), 留着只是为了两个平台共用同一套配置格式, 详见 `附 1`
> 
> `log_dir` 在 OpenWrt 上同样生效: 它是日志的【基目录】, 日志放在它下面的 `logs` 里,
> 不写时默认就是 `/var/log/esurfing` (也就是日志在 `/var/log/esurfing/logs` 下)

### 2. 保存, 输入如下指令重启服务

```shell
# 终端
# 重启服务
/etc/init.d/esurfingclient restart
# 开启自启
/etc/init.d/esurfingclient enable
```

> [!NOTE]
> v2.1.0 起每个账号会起一个独立的认证进程, 一个账号出问题不会影响其它账号
> 
> 停止或重启服务时, 认证进程会先登出再退出, 不会把账号丢在服务端在线状态

## 四、启动服务 (LuCI 方式)

### 1. 重新登录 OpenWRT 后台

### 2. 找到 `服务` -> `ESurfing 客户端`

### 3. 填写认证信息

### 4. 右下角保存并应用

### 5. 欧克

## 附 1: 参数详解

- enabled(布尔值): 程序是否启动
- log_lv(整形值, 有效范围 0-6): 日志等级, 等级越高日志显示内容越多, 数值为 0 时不输出任何日志
- conn_timeout(整形值): 自定义 CURL 连接超时时长
- op_timeout(整形值): 自定义 CURL 总操作超时时长
- accounts(数组): 账号数组
- username(字符串值): 账号
- password(字符串值): 密码
- channel(整形值, 有效范围 1-5): 认证通道
- mark(字符串值): 标记值 (高级功能)
- time_windows(字符串值): 时间控制, 可选, 数组; 每项格式 `{ "start": "mon 08:13", "end": "mon 23:57" }`, 支持跨天/跨周, 留空表示不限; 按系统本地时间判断

> [!NOTE]
> 下面这些是桌面端 (Windows / Linux / macOS) 的参数, OpenWrt 上写了也不生效, 可以不管它们:
> 
> - web_external_acc(布尔值): 网页服务是否允许外部访问 (OpenWrt 版本不带网页服务)
> - web_port(整形值): 网页服务端口 (同上)
> 
> 之所以保留在配置里, 是因为两个平台共用同一套配置格式, 配置文件直接搬过去也不会缺字段

> [!NOTE]
> `log_dir` 在 OpenWrt 上【同样生效】:
> 
> - 它是日志的**基目录**, 日志放在它下面的 `logs` 里
> - 不写 (或者写 `"."` / `"./"`) 时用默认值 `/var/log/esurfing`, 也就是日志在 `/var/log/esurfing/logs` 下
>   (随包安装的默认配置里 `log_dir` 就是 `"./"`, 所以开箱即是 `/var/log/esurfing/logs`)
> - 相对路径按 `/var/log/esurfing` 解析 (程序装在只读的 `/usr/bin` 里, 不按程序目录)
> - 想换地方就填绝对路径, 比如 `/tmp/esurfing` (日志在 `/tmp/esurfing/logs`) 或者 U 盘上的目录
> 
> ⚠️ 默认的 `/var/log` 是 tmpfs: 重启就清空, 也不磨损闪存, 小容量设备不会被日志占满。
> 改到闪存上的目录之前请想清楚容量与写次数。
> 
> 改完之后 init 脚本的日志归档与 LuCI 的日志页面都会自动跟着新目录走 (它们都是问程序要的路径)

> [!TIP]
> 配置里漏写的参数不用怕: 程序每次读取配置时会检查一遍, 缺的按默认值补上并写回配置文件, 日志里也会说明补了什么
> 
> 比如只写了 `enabled` 与 `accounts`, 剩下的参数启动一次之后就会出现在配置文件里

## 附 2: 日志与归档文件

> [!NOTE]
> 程序在 OpenWrt 上把日志写在配置里 `log_dir` 指定的目录【下面的 logs 里】, 不写时是默认的
> `/var/log/esurfing/logs`
> 
> 每次启动服务时, 上一轮的 run.log 会被归档成 `<时间戳>.log` 放在同一个目录里
> 
> 所以那个目录下的文件会随着重启变多, 这是正常的, LuCI 的日志页面可以切换查看

> [!TIP]
> 想知道日志到底写在哪, 直接在终端问程序:
> 
> ```shell
> /usr/bin/esurfingclient --print-log-dir
> ```
> 
> 它会按配置算出实际使用的日志目录并打印出来 (init 脚本的归档用的也是这个)

> [!NOTE]
> 登录成功时程序会在 `/etc/config/` 下生成 `esurfingclient.<序号>.logout`
> 
> 它的用途是: 万一程序被强杀或者设备直接断电, 下次启动时会先补一次登出, 免得账号一直卡在服务端在线, 要等服务器踢下线才能重新认证
> 
> 正常退出时会自动删掉, 不需要手动处理, 也不用去改它

## 附 3: 卸载软件包

```shell
# opkg 包管理器
opkg remove luci-app-esurfingclient esurfingclient
# apk 包管理器
apk del luci-app-esurfingclient esurfingclient
```

## 附 4: 程序服务类指令

```shell
# 服务状态
/etc/init.d/esurfingclient status
# 启动服务
/etc/init.d/esurfingclient start
# 停止服务
/etc/init.d/esurfingclient stop
# 重启服务
/etc/init.d/esurfingclient restart
# 开启自启
/etc/init.d/esurfingclient enable
# 关闭自启
/etc/init.d/esurfingclient disable
```
