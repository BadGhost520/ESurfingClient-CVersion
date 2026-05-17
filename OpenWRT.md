# OpenWRT 环境使用教程

> [!WARNING]
> 仅提供 v2.0.2-r3 及以后的新教程

## 一、从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 下载对应架构的 ipk 包

## 二、上传到 OpenWRT 系统安装

> [!WARNING]
> 注意使用 apk 包管理器的 OpenWRT 系统 (25.12.0-rc1 及以上版本) 必须要在终端里用指令安装
> 
> 因为 OpenWRT 自带的软件包安装默认是不带 `--allow-untrusted` 和 `--no-network` 参数的
> 
> 使用 opkg 软件包管理器的 OpenWRT 系统 (25.12.0-rc1 以下版本) 随意

```shell
# 终端安装
# opkg 包管理器 (OpenWRT 25.12.0 以下)
opkg install esurfingclient_*.ipk
# apk 包管理器 (OpenWRT 25.12.0 及以上)
apk add --allow-untrusted --no-network esurfingclient_*.apk
```

## 三、重新登录 OpenWRT 后台

### 1. 找到 `服务` -> `ESurfing 客户端`

### 2. 填写认证信息

### 参数详解

- enabled: 程序是否启动
- log_lv: 日志等级, 1-6级, 等级越高日志显示内容越多
- accounts: 账号数组
- username: 账号
- password: 密码
- channel: 认证通道 (暂时没找到具体作用)
- mark: 标记值 (高级功能)

### 3. 右下角保存并应用

### 4. 欧克

## 程序服务类指令

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
