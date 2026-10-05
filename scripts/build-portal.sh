#!/usr/bin/env bash

set -euo pipefail

SRC=${1:?用法: build-portal.sh <portal源目录> <输出目录> <tailwindcss可执行文件>}
OUT=${2:?用法: build-portal.sh <portal源目录> <输出目录> <tailwindcss可执行文件>}
TAILWIND=${3:?用法: build-portal.sh <portal源目录> <输出目录> <tailwindcss可执行文件>}

ALPINE_URL='https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js'
DAISYUI_URL='https://github.com/saadeghi/daisyui/releases/latest/download/daisyui.mjs'
CDN_PATTERN='cdn\.jsdelivr\.net|unpkg\.com|cdnjs\.cloudflare\.com|fonts\.googleapis\.com'

[ -d "$SRC" ] || { echo "[错误] 找不到 portal 源目录: $SRC" >&2; exit 1; }

# tailwindcss 以相对路径传入时, 转成绝对路径, 免得后面 cd 或子 shell 迷路
case $TAILWIND in
    /*) ;;
    *) TAILWIND=$PWD/$TAILWIND ;;
esac

fetch() {
    if command -v wget >/dev/null 2>&1; then
        wget --tries=5 --waitretry=3 -O "$2" "$1"
    else
        curl -fL --retry 5 --retry-delay 3 -o "$2" "$1"
    fi
}

sed_inplace() {
    sed -i.bak -e "$1" "$2"
    rm -f "$2.bak"
}

rm -rf "$OUT"
cp -r "$SRC" "$OUT"
mkdir -p "$OUT/assets/css"

fetch "$ALPINE_URL" "$OUT/assets/js/alpine.js"
fetch "$DAISYUI_URL" "$OUT/daisyui.mjs"

"$TAILWIND" -i "$OUT/input.css" -o "$OUT/assets/css/tailwind.css" --minify
rm -f "$OUT/input.css" "$OUT/daisyui.mjs"

sed_inplace '/daisyui@5\/themes.css/d' "$OUT/index.html"
sed_inplace 's#https://cdn.jsdelivr.net/npm/daisyui@5#assets/css/tailwind.css#g' "$OUT/index.html"
sed_inplace 's#<script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4"></script>##g' "$OUT/index.html"
sed_inplace 's#https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js#assets/js/alpine.js#g' "$OUT/index.html"

if grep -nE "$CDN_PATTERN" "$OUT/index.html"; then
    echo "[错误] $OUT/index.html 里还有 CDN 引用, 脱机时会加载失败" >&2
    exit 1
fi
grep -q 'href="assets/css/tailwind.css"' "$OUT/index.html" || { echo "[错误] index.html 未引用 assets/css/tailwind.css" >&2; exit 1; }
grep -q 'src="assets/js/alpine.js"' "$OUT/index.html" || { echo "[错误] index.html 未引用 assets/js/alpine.js" >&2; exit 1; }
test -s "$OUT/assets/css/tailwind.css" || { echo "[错误] tailwind.css 未生成" >&2; exit 1; }
test -s "$OUT/assets/js/alpine.js" || { echo "[错误] alpine.js 未下载" >&2; exit 1; }

echo "web 文件处理完成 (CDN 引用已全部替换为本地文件)"
ls -l "$OUT/assets/css/tailwind.css" "$OUT/assets/js/alpine.js"
