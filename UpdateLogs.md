# 更新日志

### 2025.11.03

1. 更新 OpenWrt-Arm64-MTFilogic-24.10.4(内核版本 6.6.110) 的 ipk 包
2. 更新 OpenWrt-Mipsel_24kc-MT7621-24.10.4(内核版本 6.6.110) 的 ipk 包

### 2025.11.10

1. 优化程序启动参数，现在`通道选择`参数为可选参数(默认为 pc 通道)，使用 -cpc 或 -cphone 切换相应通道

### 2025.11.14

1. 优化程序逻辑，修复了在 OpenWrt 系统中程序不能正确接收到 /etc/init.d/esurfingclient stop 的 SIGTERM 退出信号并且退出生成日志
2. 优化 ipk 包，现在更新包不会导致原配置被覆盖而需要重新填写，现在 ipk 包更新时检测到 /etc/config/esurfingclient 配置文件存在时，会生成 /etc/config/esurfingclient-opkg 配置文件而不会覆盖原配置文件
3. 修复了 OpenWrt 系统更新包时因为关闭程序和启动程序中间间隔过短导致的严重程序报错

> 关于第 2 点，就是现在 ipk 包更新不再需要重新填写个人的信息了，会自动用旧的配置

### 2025.11.16

1. 修复因为把 response_code 参数的 long 类型改成 int 类型导致的 Segmentation fault 段错误的问题
2. 尝试修复因为登录时长过久导致的 Invalid zsm data 并且无法上网的错误
3. (临时)优化日志系统，现在开启 debug 模式会让日志生成在 /usr/esurfing 目录中，而不是 /var/log/esurfing 中

### 2025.11.17

1. 优化日志系统，现在在 OpenWrt 系统开启 debug 模式会让日志生成在 /usr/esurfing 目录中，而不是在重启后就会清除的 /var/log/esurfing 目录中，其它系统仍旧
2. 将 Linux 版本做成静态链接版本，避免了因链接库版本不同导致的无法运行