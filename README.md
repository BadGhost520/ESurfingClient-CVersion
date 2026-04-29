# ESurfingClient-C

**根据 Rsplwe 大佬的 Kotlin 源码编写的纯 C 版本的 `广东` 天翼校园认证客户端** :+1:

**优点是程序文件超级小 (所有版本均是仅占用 2MB 左右的储存空间😋), 并且跨平台跨架构能力超强**

> [!NOTE]
> **(搁置中)** 现在正在做 Web 前端, 完成后可以更方便地管理程序
> 
> 可以在 **[[快照版]](https://github.com/BadGhost520/ESurfingClient-CVersion-Snapshot)** 查看我的更新进度

## 附上作者自用 K2P 路由器安装本包之后的资源占用情况⬇

![Please refresh](image/3.png) ![Please refresh](image/2.png) ![Please refresh](image/1.png)

# 目前支持的环境

| 系统      | 架构                 | (OpenWrt) 版本 |
|---------|--------------------|--------------|
| Windows | x86_64             | All          |
| Linux   | x86_64             | All          |
| OpenWrt | x86_64             | All          |
| OpenWrt | Mipsel_24kc-MT7621 | All          |
| OpenWrt | Arrch64-MTFilogic  | All          |

> [!TIP]
> 如果有其它兼容需求, 可以提交一个 issue, 会尝试进行兼容
> 
> 务必要在 issue 中提供系统和 cpu 型号, 架构等信息

# 使用教程

[**Windows & Linux 环境**](Windows&Linux.md)

[**OpenWRT 环境**](OpenWRT.md)

[**OpenWRT 进阶 - 多播**](OpenWRT_mwan3.md)

# 关于日志系统

### 在 Windows 系统中

- 程序运行后, 会在程序的运行目录下新建 logs 文件夹
- 程序运行时, logs 目录下会生成实时更新的 run.log 日志文件
- 程序退出时, run.log 日志文件会被重命名为<当前时间>.log
- 日志行数超过 10000 行会进行轮转操作 (虽然不大可能会有那么长)

### 在 Linux 系统中 (包括 OpenWRT 系统)

- 程序运行后, 会新建 /var/log/esurfing/logs 目录
- 程序运行时, logs 目录下会生成实时更新的 run.log 日志文件
- 程序退出时, run.log 日志文件会被重命名为<当前时间>.log
- 日志行数超过 10000 行会进行轮转操作 (虽然不大可能会有那么长)

# [更新日志](UpdateLogs.md)

> [!WARNING]
> 不要让我发现有人拿去做路由器贩卖喔

# 赞助 👍

觉得好的话可以点击这个[神秘小链接](https://afdian.com/a/badghost)或者下边的微信赞赏码给偶打点钱哦，谢谢泥喵~

<img alt="Please refresh" height="256" src="image/4.png" width="256"/>

# 赞助者 ❤

**感谢下面的赞助者支持👍**

<img alt="Please refresh" src="image/fund/1.png"/>

<img alt="Please refresh" src="image/fund/2.png"/>

<img alt="Please refresh" height="256" src="image/fund/3.jpg" width="256"/>

<img alt="Please refresh" height="256" src="image/fund/4.jpg" width="256"/>

<img alt="Please refresh" height="256" src="image/fund/5.jpg" width="256"/>

<img alt="Please refresh" height="256" src="image/fund/6.png" width="256"/>

<img alt="Please refresh" height="256" src="image/fund/7.png" width="256"/>
