# OpenWRT 系统目标平台自查教程

> [!NOTE]
> 教程版本: v2.0.8-r1

## 必要知识

### 目标平台是什么?

### 如下图红框所示

<img alt="Please refresh" width="75%" src="../image/targets/01.png"/>

**一个架构实际上包含多个目标平台**

**例如下载 mipsel_24kc 架构的 OpenWRT 插件安装包**

**那么它就能让以下这几个目标平台安装使用**

```text
malta/le
ramips/mt7620
ramips/mt7621
ramips/mt76x8
ramips/rt305x
```

## 架构包含的目标平台自查表

<details>
<summary>
All
</summary>

**这个是 LuCI 包专属, 表示所有目标平台都能装**

</details>

<details>
<summary>
aarch64_cortex-a53
</summary>

**aarch64_cortex-a53 架构所包含的目标平台**

```text
airoha/an7581
airoha/an7583
bcm27xx/bcm2710
bcm4908/generic
imx/cortexa53
mediatek/filogic
mediatek/mt7622
microchipsw/lan969x
mvebu/cortexa53
qualcommax/ipq807x
qualcommax/ipq60xx
qualcommax/ipq50xx
qualcommbe/ipq95xx
sunxi/cortexa53
```

</details>

<details>
<summary>
aarch64_generic
</summary>

**aarch64_generic 架构所包含的目标平台**

```text
armsr/armv8
layerscape/armv8_64b
rockchip/armv8
```

</details>

<details>
<summary>
mipsel_24kc
</summary>

**mipsel_24kc 架构所包含的目标平台**

```text
malta/le
ramips/mt7620
ramips/mt7621
ramips/mt76x8
ramips/rt305x
```

</details>

<details>
<summary>
x86_64
</summary>

**x86_64 架构所包含的目标平台**

```text
x86/64
```

> [!NOTE]
> 这 x86_64 肯定是只包含 x86/64 的呐~

</details>
