#!/usr/bin/env bash
# 把可选的 Windows 网速与热点工具复制进发布目录, 并做最基本的完整性检查。
#
# 用户拿到的包必须是"解开就能用"的: 所以这里只放运行必需的文件 (测试脚本一律不进包),
# 复制完再逐个核对 - 缺文件、混进多余文件、启动器行尾不对, 都直接让打包失败,
# 而不是等用户双击之后才发现打不开。
#
# 包根的 Start-Dashboard.cmd 是"用户在压缩包根目录双击的那个", 内容只有四行,
# 直接在这里生成, 免得工具目录里再放一个不随包发布的模板文件。
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
OUT=${1:?Usage: package-windows-helper.sh <release-directory>}
SRC="$ROOT/tools/windows-network-panel"
DEST="$OUT/tools/windows-network-panel"

# 发布包里"工具目录"应有的文件 (顺序即复制顺序)
FILES="NetworkPanel.ps1 TrafficCollector.ps1 HotspotBridge.ps1 Start-Dashboard.ps1 Start-Dashboard.cmd Start-NetworkPanel.cmd Get-ValidationEnvironment.ps1 HARDWARE-VALIDATION.md README.md"

# .cmd 一律落成 CRLF: CI 在 Linux 上检出, 只有 LF 的批处理在 cmd.exe 上可能被解析错。
# .gitattributes 已经钉了 eol=crlf, 这里再兜一次底 (就地打包时也同样生效)
to_crlf() {
    perl -pi -e 's/\r?\n/\r\n/' "$1"
}

# 先核对源文件齐全, 免得 cp 抛一句英文报错就结束
for file in $FILES; do
    [ -f "$SRC/$file" ] || { echo "[错误] 源码里缺少 tools/windows-network-panel/$file" >&2; exit 1; }
done

mkdir -p "$DEST"
DEST=$(cd -- "$DEST" && pwd)

for file in $FILES; do
    # OUT 指向仓库根时 (CI 的就地打包) 源和目标是同一个目录, 不必复制
    if [ "$SRC" != "$DEST" ]; then
        cp "$SRC/$file" "$DEST/$file"
    fi
    case "$file" in
        *.cmd) to_crlf "$DEST/$file" ;;
    esac
done

# 包根启动器: 调用 tools\windows-network-panel\Start-Dashboard.ps1, 参数原样透传
cat > "$OUT/Start-Dashboard.cmd" <<'LAUNCHER'
@echo off
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\windows-network-panel\Start-Dashboard.ps1" %*
if errorlevel 1 pause
LAUNCHER
to_crlf "$OUT/Start-Dashboard.cmd"

# 完整性检查: 文件齐全 + 没有多余文件 (测试脚本之类不该进包)
for file in $FILES; do
    [ -f "$DEST/$file" ] || { echo "[错误] 发布包缺少 tools/windows-network-panel/$file" >&2; exit 1; }
done
EXTRA=$(cd "$DEST" && ls -A | grep -v -x -F -f <(printf '%s\n' $FILES) || true)
if [ -n "$EXTRA" ]; then
    echo "[错误] 发布包的工具目录里有多余文件 (测试脚本不应进包): $EXTRA" >&2
    exit 1
fi
[ -f "$OUT/Start-Dashboard.cmd" ] || { echo "[错误] 发布包根目录缺少 Start-Dashboard.cmd" >&2; exit 1; }

echo "Windows 辅助工具已打包: tools/windows-network-panel ($(printf '%s\n' $FILES | wc -l) 个文件) + 包根 Start-Dashboard.cmd"
