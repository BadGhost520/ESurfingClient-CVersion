# ESurfingClient-C

**根据 Rsplwe 大蛇的 Kotlin 源码编写的 C 语言版本的天翼校园认证客户端** :+1:

> [!IMPORTANT]
> 这是一个`半开源`程序，隐藏了加解密算法代码

# 目前支持的环境

|系统|架构|版本|
|----|----|----|
|Windows|x64|All|
|Linux|x64|All|
|OpenWRT|mipsel_24kc|21.02.7, 23.05.0|

# 可选的认证通道

1. pc (Linux)
2. phone (Android)

# Windows & Linux 环境使用教程

**1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases) 中下载相应的程序**

**2. 将程序放在自己想要的位置**

**3. 运行如下命令以启动程序**

```shell
./ESurfingClient -u <用户名> -p <密码> -c <认证通道>
```

```bash
.\ESurfingClient.exe -u <用户名> -p <密码> -c <认证通道>
```

# OpenWRT 环境使用教程

**1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases) 中下载相应的 ipk 包**

**2. 使用 opkg install 安装下载的 ipk 包**

**3. 运行下面`必要`的命令以设置服务配置文件和运行服务**

```bash
# 设置用户名 (必要)
uci set esurfingclient.main.username='<用户名>'
```
```bash
# 设置密码 (必要)
uci set esurfingclient.main.password='<密码>'
```
```bash
# 设置程序是否能被启动 (必要)
uci set esurfingclient.main.enabled='1'
```
```bash
# 设置认证通道 (可选)
uci set esurfingclient.main.channel='<认证通道>'(默认为`pc`)
```
```bash
# 提交更改 (必要)
uci commit esurfingclient
```
```bash
# 重载配置文件 (必要)
/etc/init.d/esurfingclient reload
```
```bash
# 设置开机自启 (可选)
/etc/init.d/esurfingclient enable
```
```bash
# 停止服务
/etc/init.d/esurfingclient stop
```
```bash
# 启动服务
/etc/init.d/esurfingclient start
```

# 关于日志文件

> [!IMPORTANT]
> 在 Windows 系统中
> 
> 程序运行后，会在运行目录下新建 logs 文件夹
> 
> 程序运行时，logs 文件夹内会生成实时更新的 run.log 日志文件
> 
> 程序退出时，run.log 日志文件会被重命名为<当前时间>.log
>
> 在 Linux 类系统中
>
> 程序运行后，会在 /var/log 目录下下新建 esurfing/logs 目录
> 
> 程序运行时，logs 文件夹内会生成实时更新的 run.log 日志文件
> 
> 程序退出时，run.log 日志文件会被重命名为<当前时间>.log
