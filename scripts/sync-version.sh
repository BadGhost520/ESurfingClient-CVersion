#!/usr/bin/env bash

set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
CMAKE_LISTS="$ROOT/esurfingclient/main/CMakeLists.txt"
MK_MAIN="$ROOT/esurfingclient/Makefile"
MK_LUCI="$ROOT/luci-app-esurfingclient/Makefile"
JS="$ROOT/luci-app-esurfingclient/rootfs/www/luci-static/resources/view/esurfingclient.js"
HTM="$ROOT/luci-app-esurfingclient/rootfs-legacy/usr/lib/lua/luci/view/esurfingclient.htm"

usage() {
    echo "用法: scripts/sync-version.sh [<major>.<minor>.<patch>-r<release> | --print]" >&2
}

# sed -i 在 GNU 与 BSD 上写法不同, 统一成一种
sed_inplace() {
    sed -i.bak -e "$1" "$2"
    rm -f "$2.bak"
}

# 读 CMakeLists 里的 set(<名字> <数字>)
read_var() {
    sed -n "s/^[[:space:]]*set($1[[:space:]][[:space:]]*\([0-9][0-9]*\))[[:space:]]*$/\1/p" "$CMAKE_LISTS" | head -n 1
}

PRINT_ONLY=0
NEW_VERSION=""
for arg in "$@"; do
    case $arg in
        --print)   PRINT_ONLY=1 ;;
        -h|--help) usage; exit 0 ;;
        -*)        echo "[错误] 未知参数: $arg" >&2; usage; exit 1 ;;
        *)         NEW_VERSION=$arg ;;
    esac
done

if [ "$PRINT_ONLY" = 1 ] && [ -n "$NEW_VERSION" ]; then
    echo "[错误] --print 是只读模式, 不能同时指定版本号" >&2
    exit 1
fi

[ -f "$CMAKE_LISTS" ] || { echo "[错误] 找不到版本真相源: $CMAKE_LISTS" >&2; exit 1; }

# 给了版本号就先落到真相源, 之后的流程完全一样 (真相源始终是那四行)
if [ -n "$NEW_VERSION" ]; then
    case $NEW_VERSION in
        [0-9]*.[0-9]*.[0-9]*-r[0-9]*) ;;
        *) echo "[错误] 版本号格式必须是 <major>.<minor>.<patch>-r<release>, 例如 2.2.0-r1 (收到: $NEW_VERSION)" >&2; exit 1 ;;
    esac
    V=${NEW_VERSION%-r*}
    R=${NEW_VERSION##*-r}
    MAJOR=${V%%.*}
    REST=${V#*.}
    MINOR=${REST%%.*}
    PATCH=${REST#*.}
    sed_inplace "s/^set(PROGRAM_VERSION_MAJOR .*/set(PROGRAM_VERSION_MAJOR $MAJOR)/" "$CMAKE_LISTS"
    sed_inplace "s/^set(PROGRAM_VERSION_MINOR .*/set(PROGRAM_VERSION_MINOR $MINOR)/" "$CMAKE_LISTS"
    sed_inplace "s/^set(PROGRAM_VERSION_PATCH .*/set(PROGRAM_VERSION_PATCH $PATCH)/" "$CMAKE_LISTS"
    sed_inplace "s/^set(PROGRAM_VERSION_RELEASE .*/set(PROGRAM_VERSION_RELEASE $R)/" "$CMAKE_LISTS"
fi

MAJOR=$(read_var PROGRAM_VERSION_MAJOR)
MINOR=$(read_var PROGRAM_VERSION_MINOR)
PATCH=$(read_var PROGRAM_VERSION_PATCH)
RELEASE=$(read_var PROGRAM_VERSION_RELEASE)

for val in "$MAJOR" "$MINOR" "$PATCH" "$RELEASE"; do
    case $val in
        ""|*[!0-9]*)
            echo "[错误] CMakeLists.txt 里的版本号读不出来或不是纯数字:" >&2
            echo "        MAJOR='$MAJOR' MINOR='$MINOR' PATCH='$PATCH' RELEASE='$RELEASE'" >&2
            echo "        期望形如 set(PROGRAM_VERSION_MAJOR 2)" >&2
            exit 1
            ;;
    esac
done

VERSION="$MAJOR.$MINOR.$PATCH"
FULL_VERSION="$VERSION-r$RELEASE"

if [ "$PRINT_ONLY" = 1 ]; then
    # 只输出 key=value, 好让 workflow 直接 >> "$GITHUB_OUTPUT"
    echo "version=$VERSION"
    echo "release=$RELEASE"
    echo "full_version=$FULL_VERSION"
    exit 0
fi

echo "版本真相源 esurfingclient/main/CMakeLists.txt: $FULL_VERSION"

# 1) 两个包的 Makefile
sed_inplace "s/^PKG_VERSION:=.*/PKG_VERSION:=$VERSION/" "$MK_MAIN"
sed_inplace "s/^PKG_RELEASE:=.*/PKG_RELEASE:=$RELEASE/" "$MK_MAIN"
sed_inplace "s/^PKG_VERSION:=.*/PKG_VERSION:=$VERSION/" "$MK_LUCI"
sed_inplace "s/^PKG_RELEASE:=.*/PKG_RELEASE:=$RELEASE/" "$MK_LUCI"
echo "已同步: esurfingclient/Makefile, luci-app-esurfingclient/Makefile"

# 2) LuCI 页面里显示的版本号
[ -f "$JS" ] || { echo "[错误] 找不到 LuCI 页面: $JS" >&2; exit 1; }
[ -f "$HTM" ] || { echo "[错误] 找不到传统 LuCI 页面: $HTM" >&2; exit 1; }
sed_inplace "s/E('p', { class: 'desc' }, 'LuCI 版本.*/E('p', { class: 'desc' }, 'LuCI 版本: $FULL_VERSION'),/" "$JS"
sed_inplace "s%<p class='desc'>LuCI 版本.*%<p class='desc'>LuCI 版本: $FULL_VERSION</p>%" "$HTM"
echo "已同步: LuCI 页面版本号"

# 3) 复检: 分发结果必须与真相源一致, 不一致宁可报错
for f in "$MK_MAIN" "$MK_LUCI"; do
    grep -q "^PKG_VERSION:=$VERSION\$" "$f" || { echo "[错误] $f 的 PKG_VERSION 未同步" >&2; exit 1; }
    grep -q "^PKG_RELEASE:=$RELEASE\$" "$f" || { echo "[错误] $f 的 PKG_RELEASE 未同步" >&2; exit 1; }
done
grep -q "LuCI 版本: $FULL_VERSION" "$JS" || { echo "[错误] $JS 的版本号未同步" >&2; exit 1; }
grep -q "LuCI 版本: $FULL_VERSION" "$HTM" || { echo "[错误] $HTM 的版本号未同步" >&2; exit 1; }
echo "自检通过: 四处分发结果与 CMakeLists 一致"
