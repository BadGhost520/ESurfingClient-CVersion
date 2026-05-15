# OpenWRT 环境使用教程

> [!WARNING]
> 更新 v2 版本后此教程仅提供 v2 版本的教程
> 
> 如若需要 v1 版本的教程, 可自行前往 v1 分支查看

## v2 版本的安装和使用都十分简单, 跟着一步一步即可

### 1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 下载对应架构的 ipk 包

### 2. 上传到 OpenWRT 系统安装

> [!WARNING]
> 注意使用 apk 包管理器的 OpenWRT 系统必须要在终端里用指令安装
> 
> OpenWRT 自带的软件包安装默认是不带 `--allow-untrusted` 和 `--no-network` 参数的

```shell
# opkg 包管理器 (OpenWRT 25.12.0 以下)
opkg install esurfingclient_*.ipk
# apk 包管理器 (OpenWRT 25.12.0 及以上)
apk add --allow-untrusted --no-network esurfingclient_*.apk
```

### 3. 修改位于 /etc/config/esurfingclient 的配置文件

> [!NOTE]
> 2.0.0-r13 以下是在 `/usr/bin/ESurfingClient.json`, 且没有 `enabled` 参数
> 
> 2.0.0-r13 及以上才是在 `/etc/config/esurfingclient`

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
> 将 `enabled` 值从 false 改为 true (2.0.0-r13 以下)
> 
> 将 `username` 和 `password` 两栏填入然后保存即可, 两个值对应的是官方客户端的账密

### 4. 确保前面步骤无误之后输入如下指令即可让程序使用新的配置文件启动

```shell
# 重启服务
/etc/init.d/esurfingclient restart
```

## 其它程序服务类指令

```shell
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

## JSON 参数解释

- enabled: 程序是否启动
- log_lv: 日志等级, 1-6级, 等级越高日志显示内容越多
- accounts: 账号数组
- username: 账号
- password: 密码
- channel: 认证通道 (暂时没找到具体作用)
