# Windows & Linux 环境使用教程

**1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 中下载相应的程序**

**2. 将程序放在自己想要的位置**

**3. 运行如下命令以启动程序**

### Linux
```bash
./ESurfingClient-x86_64-linux -u <用户名> -p <密码>
```

### Windows
```shell
.\ESurfingClient-x86_64-windows.exe -u <用户名> -p <密码>
```

# 示例

### Linux
```bash
./ESurfingClient-x86_64-linux -u 233333333 -p A1234567
```

### Windows
```shell
.\ESurfingClient-x86_64-windows.exe -u 233333333 -p A1234567
```

> [!TIP]
> 目前有两个认证通道: pc 和 phone
> 
> **两者并没有什么太大的区别**
> 
> 如果需要切换认证通道，可以在命令最后添加 -cpc (PC 通道)或者 -cphone (Phone 通道)来切换
> 
> 默认为 PC 通道

# 示例

### Linux
```bash
./ESurfingClient-x86_64-linux -u 233333333 -p A1234567 -cpc
./ESurfingClient-x86_64-linux -u 233333333 -p A1234567 -cphone
```

### Windows
```shell
.\ESurfingClient-x86_64-windows.exe -u 233333333 -p A1234567 -cpc
.\ESurfingClient-x86_64-windows.exe -u 233333333 -p A1234567 -cphone
```
