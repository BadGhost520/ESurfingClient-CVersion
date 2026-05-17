# OpenWRT 进阶 - 多播教程

> [!WARNING]
> 本教程为 v2 版本专属, v1 版本不支持该操作
> 
> 教程中的 OpenWRT 版本为 24.10.5, 旧版本其中一些步骤会有些许不同, 需自行查找资料对比修改
> 
> 教程中使用的是双网线双 WAN 多播, 如需单线多播可能需要寻找其它对应资料实现
> 
> 通过 OpenWRT 插件 mwan3 完成
>
> 需要自行提前为 OpenWRT 安装好 mwan3 插件

### 1. 检查 mwan3 插件是否安装

<img alt="Please refresh" width="75%" src="image/mwan3/01.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/02.png"/>

### 2. 按照图示删除原来的 wan 口, 点保存并应用

<img alt="Please refresh" width="75%" src="image/mwan3/03.png"/>

### 3. 切换到设备栏, 修改 br-lan 配置

<img alt="Please refresh" width="75%" src="image/mwan3/04.png"/>

### 4. 取消最后一个 lan 口, 随后保存并应用

> [!NOTE]
> 取消最后一个 lan 口的原因是, 我路由器的 lan4 口靠近 wan 口
> 
> 实际取消哪个都可以, 只需保证后面匹配上即可

<img alt="Please refresh" width="75%" src="image/mwan3/05.png"/>

### 5. 回到接口栏, 点击 `添加新接口`

<img alt="Please refresh" width="75%" src="image/mwan3/06.png"/>

### 6. 创建一个 wan1 口, 协议 DHCP 客户端, 设备选择 wan 口, 然后点击创建接口

<img alt="Please refresh" width="75%" src="image/mwan3/07.png"/>

### 7. 重要的一点, 选择高级设置栏, 将下面的网关跃点设为 10

<img alt="Please refresh" width="75%" src="image/mwan3/08.png"/>

### 8. 选择防火墙设置栏, 分配 wan 区域防火墙, 然后点击保存

<img alt="Please refresh" width="75%" src="image/mwan3/09.png"/>

### 9. 创建 wan2 口, 与 wan1 差不多, 不过设备要选择刚刚从 br-lan 那取消的那个 lan 口, 网关跃点设置为 20

<img alt="Please refresh" width="75%" src="image/mwan3/10.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/11.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/12.png"/>

### 10. 确定无误后点击保存并应用, 插入网线观察是否获取到正确的校园网 IP 地址 (我的是 172.19 开头的)

<img alt="Please refresh" width="75%" src="image/mwan3/13.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/14.png"/>

### 11. 接下来是 mwan3 操作, 按照图示切换到 mwan 管理器的接口栏, 并删除所有默认接口

<img alt="Please refresh" width="75%" src="image/mwan3/15.png"/>

### 12. 随后添加 wan1 接口, 按照图示进行配置, wan2 接口同理

<img alt="Please refresh" width="75%" src="image/mwan3/16.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/17.png"/>

### 13. 确定无误后点击保存并应用

<img alt="Please refresh" width="75%" src="image/mwan3/18.png"/>

### 14. 切换到成员栏, 并删除所有默认成员

<img alt="Please refresh" width="75%" src="image/mwan3/19.png"/>

### 15. 随后添加 wan1_m1 成员, 按照图示进行配置, wan2_m2 成员同理

<img alt="Please refresh" width="75%" src="image/mwan3/20.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/21.png"/>

### 16. 确定无误后点击保存并应用

<img alt="Please refresh" width="75%" src="image/mwan3/22.png"/>

### 17. 切换到策略栏, 并删除除 balanced 策略以外的所有默认策略

<img alt="Please refresh" width="75%" src="image/mwan3/23.png"/>

### 18. 点击编辑 balanced 策略, 按照图示进行配置

<img alt="Please refresh" width="75%" src="image/mwan3/24.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/25.png"/>

### 19. 确定无误后点击保存并应用

<img alt="Please refresh" width="75%" src="image/mwan3/26.png"/>

### 20. 切换到规则栏, 并删除 default_rule_v4 以外的所有默认规则

<img alt="Please refresh" width="75%" src="image/mwan3/27.png"/>

### 21. 点击编辑 default_rule_v4 规则, 开启粘滞模式, 随后保存

<img alt="Please refresh" width="75%" src="image/mwan3/28.png"/>

<img alt="Please refresh" width="75%" src="image/mwan3/29.png"/>

### 22. 确定无误后点击保存并应用

<img alt="Please refresh" width="75%" src="image/mwan3/30.png"/>

### 23. 按照图示观察, wan1 和 wan2 状态为离线即为配置好 mwan3 了

<img alt="Please refresh" width="75%" src="image/mwan3/31.png"/>

### 24. 安装认证程序 - [下载传送门](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest)

### 25. 退出并重新登录后台, 打开 `服务` >> `ESurfing 客户端`

### 26. 填写好认证配置 (`标记值` 的用途 > [传送门](#v202-r3-版本新增-标记值))

<img alt="Please refresh" width="75%" src="image/mwan3/32.png"/>

### 27. 运行程序前的检查, 输入 ip rule show 检查是否有如图红框的规则, 有则正常, 没有则检查前面是否有步骤做错了

<img alt="Please refresh" width="75%" src="image/mwan3/33.png"/>

### 28. 输入 /etc/init.d/esurfingclient reload 重载 init.d 配置文件, 输入 tail -f -n 50 /var/log/esurfing/logs/run.log 查看运行情况

<img alt="Please refresh" width="75%" src="image/mwan3/34.png"/>

> [!NOTE]
> 如果一切正常且下游设备有网了
> 
> 那么就恭喜你实现了双 WAN 口多播
> 
> 上行和下行在 `多线程` 上传和下载时都会受到叠加 buff

## v2.0.2-r3 版本新增 (标记值)

### 这是一个用于给一些特殊网络环境使用的方案

#### 举个例子

#### 假如 wan1 口接的是不用认证就有网的网络, wan2 则是需要认证的校园网络

#### 小明按先 wan1, 后 wan2 的顺序填写了 mwan3 的 `接口` 配置

#### 然后填写了 `仅一个` ESurfing 客户端的认证配置并启动 ESurfing 客户端, 发现不行, 无法认证, 总提示 `已连接到互联网`

#### 因为 wan1 和 wan2 会被 mwan3 按填写顺序分配 0x100 和 0x200 的标记值

#### wan1 只允许 0x100 和没标签的数据包通过, wan2只允许 0x200 和没标签的数据包通过

#### 而 ESurfing 客户端的自动分配标记值会按照可用配置按顺序打标签

#### 即, 可用配置 1 打上 0x100, 可用配置 2 打上 0x200, 以此类推

#### 小明的认证配置无法进行认证, 总提示 `已连接到互联网` 的原因就是这个

#### 只填了配置 1, 然后程序自动打上 0x100 的标签, 数据包就跑到 wan1 有网的口去了

### 所以! 隆重推出我们的自定义标记值功能! 👏

<img alt="Please refresh" width="75%" src="image/mwan3/35.png"/>

### 只需要在配置页给每个认证配置填写上标记值, 对应的配置在发包的时候就会被打上对应的标签

### 然后数据包就会乖乖地按照被打上的标签去走对应的通道

### 妈妈再也不用担心我的数据包乱跑辣!

> [!WARNING]
> 标记值是一个十六进制数, 以 0x 开头, 而且必须是 0x100 的倍数
> 
> 可以直接在终端用 ip rule show 来查看要打什么标签
> 
> 以免数据包标签管理混乱, 只要有一个认证配置填了自定义标记值
> 
> 那么其它的认证配置就都要填, 不然会被程序跳过加载
