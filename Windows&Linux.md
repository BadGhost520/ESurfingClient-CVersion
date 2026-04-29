# Windows & Linux 环境使用教程

> [!WARNING]
> 更新 v2 版本后此教程仅提供 v2 版本的教程
>
> 如若需要 v1 版本的教程, 可自行前往 v1 分支查看

## v2 版本的使用十分简单, 跟着一步一步即可

### 1. 从 [Release](https://github.com/BadGhost520/ESurfingClient-CVersion/releases/latest) 中下载相应的程序

### 2. 将程序放在自己想要的位置

### 3. Windows 双击直接运行, Linux 也是直接无参数直接执行

```shell
# Windows
.\ESurfingClient-x86_64-windows.exe
```
```shell
# Linux
./ESurfingClient-x86_64-linux
```

> [!NOTE]
> 运行之后会在用户所在目录生成一个 ESurfingClient.json 配置文件
> 
> 何为用户所在目录, 如下所示

```shell
# Windows CMD
C:\Users\bad_g>
```
```shell
# Linux Shell
badghost@BadGhost:~$
```

> [!NOTE]
> 所以 Windows 用双击执行是最方便的

### 4. 修改生成的 ESurfingClient.json

```json
{
  "log_lv": 4,
  "accounts": [
    {
      "username": "在这填账号",
      "password": "在这填密码",
      "channel": "phone"
    }
  ]
}
```

### 5. 填完保存再次运行即可

## JSON 参数解释

- log_lv: 日志等级, 1-6级, 等级越高日志显示内容越多
- accounts: 账号数组
- username: 账号
- password: 密码
- channel: 认证通道 (暂时没找到具体作用)
