# 更新日志

### 2025.11.03

1. 更新 OpenWrt-Arm64-MTFilogic-24.10.4(内核版本 6.6.110) 的 ipk 包
2. 更新 OpenWrt-Mipsel_24kc-MT7621-24.10.4(内核版本 6.6.110) 的 ipk 包

### 2025.11.10

1. 优化程序启动参数，现在`通道选择`参数为可选参数(默认为 pc 通道)，使用 -cpc 或 -cphone 切换相应通道

### 2025.11.14

1. 优化程序逻辑，修复了在 OpenWrt 系统中程序不能接收到 /etc/init.d/esurfingclient stop 的 SIGTERM 退出信号并且正确退出生成日志
2. 优化 ipk 包，现在更新包不会导致原配置被覆盖而需要重新填写，现在 ipk 包更新时检测到 /etc/config/esurfingclient 配置文件存在时，会生成 /etc/config/esurfingclient-opkg 配置文件而不会覆盖原配置文件
3. 修复了 OpenWrt 系统更新包时因为关闭程序和启动程序中间间隔过短导致的严重程序报错

> 关于第 2 点，就是现在 ipk 包更新不再需要重新填写个人的信息了，会自动用旧的配置