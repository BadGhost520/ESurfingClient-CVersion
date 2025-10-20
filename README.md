# ESurfingClient-C

**根据 Rsplwe 的 Kotlin 版本编写的 C 语言版本的天翼校园认证客户端** :+1:

> [!IMPORTANT]
> 这是一个`半开源`程序，隐藏了加解密算法代码

# 目前支持的环境

|系统|架构|版本|
|----|----|----|
|Windows|x64|All|
|Linux|x64|All|
|OpenWRT|mipsel_24kc|21.02.7, 23.05.0|

# Windows & Linux 环境使用教程

**1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases) 中下载相应的程序**

**2. 将程序放在自己想要的位置**

**3. 执行如下命令以启动程序**

```shell
./ESurfingClient -u <用户名> -p <密码> -c <通道>
```

```bash
.\ESurfingClient.exe -u <用户名> -p <密码> -c <通道>
```

> [!NOTE]
> 可选的认证通道: 
> 1. pc (Linux)
> 2. phone (Android)

> [!IMPORTANT]
> 程序运行后，会在运行目录下新建 logs 文件夹
> 
> 程序运行时，logs 文件夹内会生成实时更新的 run.log 日志文件
> 
> 程序退出时，run.log 日志文件会被重命名为<当前时间>.log

# OpenWRT 
