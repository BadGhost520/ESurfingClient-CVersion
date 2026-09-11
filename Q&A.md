<details>
<summary>
Q: 除了广东地区的电信校园网可以用吗?
</summary>

**A: 目前来看, 应该大部分是不行的, 因为每个省的认证流程都有差别, 认证服务器可能都不同**

**如果抓包对比流程差不多的话, 可以自行 fork 并改改流程来用**

</details>

<details>
<summary>
Q: 为什么我会出现 URL 格式错误的报错并且认证不了? (参考日志)
</summary>

```text
# 一段日志
[xxxx-xx-xx xx:xx:xx] [TID xxx] [T-0] [ERROR] [NetClient.c:xx] curl 错误码: 3, 错误原因: URL 格式错误
[xxxx-xx-xx xx:xx:xx] [TID xxx] [T-0] [DEBUG] [NetClient.c:xx] 配置 1 获取认证配置 URL: /qs/main.jsp?wlanacip=**.**.**.**&wlanuserip=**.**.**.**
```
**A: 这是 UA2F/UA3F 开启之后的典型特征, 关闭即可, 或根据以下原理修改 UA2F/UA3F 配置**

- UA2F/UA3F 会将经过路由器的 HTTP 包的 UA 修改为指定 UA (默认为 F)
- 电信校园网获取认证配置靠指定 UA 获取 (例如 CCTP/android11_64/2104)
- [参考 Issue](https://github.com/BadGhost520/ESurfingClient-CVersion/issues/30)

</details>

<details>
<summary>
Q: 为什么在使用 opkg 管理器的版本里推荐 19.07.0 这个版本
</summary>

**A: 因为它是开始使用 LuCI2 的第一个版本**

</details>
