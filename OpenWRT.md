# OpenWRT 环境使用教程

**1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 中下载自己需要的 ipk 包**

**2. 使用 opkg install 安装下载的 ipk 包**

> [!NOTE]
> 可能会缺少一些依赖包，需要手动从镜像站上面下载
> 
> 这里提供一个好用的镜像站地址: [南方科技大学镜像站](https://mirrors.sustech.edu.cn/openwrt/releases/)

**3. `按顺序` 运行下面 `必要` 的命令以设置服务配置文件和运行服务**

> [!WARNING]
> **一定要按顺序！**

### 必要指令

```bash
# 1. 设置用户名
uci set esurfingclient.main.username='<用户名>'
# 示例
uci set esurfingclient.main.username='23333333'
```
```bash
# 2. 设置密码
uci set esurfingclient.main.password='<密码>'
# 示例
uci set esurfingclient.main.password='A1234567'
```
```bash
# 3. 设置程序是否能被启动
uci set esurfingclient.main.enabled='1'
```
```bash
# 4. 提交更改
uci commit esurfingclient
```
```bash
# 5. 重载配置文件
/etc/init.d/esurfingclient reload
```

### 可选指令

```bash
# 设置认证通道(默认为`pc`)
uci set esurfingclient.main.channel='<认证通道>'
# 示例
uci set esurfingclient.main.channel='pc'
```

> [!TIP]
> 目前有两个认证通道: pc 和 phone
> 
> **两者并没有什么太大的区别**

```bash
# 设置调试模式(默认关闭)
uci set esurfingclient.main.debug='1 or 0'
# 示例
uci set esurfingclient.main.debug='0'
```

```bash
# 设置小容量设备模式(默认开启)
uci set esurfingclient.main.smallDevice='1 or 0'
# 示例
uci set esurfingclient.main.smallDevice='1'
```

### 程序服务类指令

```bash
# 设置开机自启
/etc/init.d/esurfingclient enable
```
```bash
# 重启服务
/etc/init.d/esurfingclient restart
```
```bash
# 停止服务
/etc/init.d/esurfingclient stop
```
```bash
# 启动服务
/etc/init.d/esurfingclient start
```
