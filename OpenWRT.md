# OpenWRT 环境使用教程

**1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 中下载自己需要的 ipk 包**

**2. 使用 opkg install 安装下载的 ipk 包**

**3. 运行下面`必要`的命令以设置服务配置文件和运行服务**

```bash
# 设置用户名 (必要)
uci set esurfingclient.main.username='<用户名>'
# 示例
uci set esurfingclient.main.username='23333333'
```
```bash
# 设置密码 (必要)
uci set esurfingclient.main.password='<密码>'
# 示例
uci set esurfingclient.main.password='A1234567'
```
```bash
# 设置程序是否能被启动 (必要)
uci set esurfingclient.main.enabled='1'
```
```bash
# 设置认证通道 (可选)
uci set esurfingclient.main.channel='<认证通道>'(默认为`pc`)
# 示例
uci set esurfingclient.main.channel='pc'
```

> [!TIP]
> 目前有两个认证通道: pc 和 phone
> 
> **两者并没有什么太大的区别**

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
