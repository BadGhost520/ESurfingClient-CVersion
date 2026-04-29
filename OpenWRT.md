# OpenWRT 环境使用教程

> [!WARNING]
> 更新 v2 版本后此教程仅提供 v2 版本的教程
> 
> 如若需要 v1 版本的教程, 可自行前往 v1 分支查看

## v2 版本的安装和使用都十分简单, 跟着一步一步即可

### 1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 下载对应架构的 ipk 包

### 2. 上传到 OpenWRT 系统安装

### 3. 修改位于 /usr/bin/ESurfingClient.json 的配置文件

```json
{
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
> 将 username 和 password 两栏填入然后保存即可

### 4. 修改位于 /etc/config/esurfingclient 的配置文件

```config
config esurfingclient 'main'
	option enabled '0' # <- 这个 '0' 要改成 '1' 程序才能被允许运行
```

### 5. 确保前面步骤无误之后输入如下指令即可运行程序

```shell
/etc/init.d/esurfingclient reload # 重载配置文件
/etc/init.d/esurfingclient enable # 设置开机自启
```

## 其它程序服务类指令

```shell
# 启动服务
/etc/init.d/esurfingclient start
```
```shell
# 停止服务
/etc/init.d/esurfingclient stop
```
```shell
# 重启服务
/etc/init.d/esurfingclient restart
```

## JSON 参数解释

- log_lv: 日志等级, 1-6级, 等级越高日志显示内容越多
- accounts: 账号数组
- username: 账号
- password: 密码
- channel: 认证通道 (暂时没找到具体作用)
