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
  "log_lv": 4,
  "conn_timeout": 7,
  "op_timeout":   10,
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

## 附 2: 日志与归档文件

> [!NOTE]
> 每次启动服务时, 上一轮的 run.log 会被归档成 `<时间戳>.log` 放在同一个目录里
> 
> 所以 `/var/log/esurfing/logs` 下的文件会随着重启变多, 这是正常的, LuCI 的日志页面可以切换查看

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
