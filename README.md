# ESurfingClient-C

**根据 Rsplwe 大蛇的 Kotlin 源码编写的 C 语言版本的天翼校园认证客户端** :+1:

> [!IMPORTANT]
> 这是一个`半开源`程序，隐藏了加解密算法代码

# 目前支持的环境

|系统|架构|版本|
|----|----|----|
|Windows|x86_64|All|
|Linux|x86_64|All|
|OpenWRT|x86_64|21.02.7, 22.03.7, 24.10.4|
|OpenWRT|mipsel_24kc|21.02.7, 23.05.0|

# 使用教程

[**Windows & Linux 环境**](./Windows&Linux.md)

[**OpenWRT 环境**](./OpenWRT.md)

# 关于日志系统

### 在 Windows 系统中

- 程序运行后，会在程序的运行目录下新建 logs 文件夹

- 程序运行时，logs 目录下会生成实时更新的 run.log 日志文件

- 程序退出时，run.log 日志文件会被重命名为<当前时间>.log

### 在 Linux 类系统中

- 程序运行后，会新建 /var/log/esurfing/logs 目录

- 程序运行时，logs 目录下会生成实时更新的 run.log 日志文件

- 程序退出时，run.log 日志文件会被重命名为<当前时间>.log
