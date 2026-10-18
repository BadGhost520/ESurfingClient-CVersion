#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
OUT=${1:?Usage: package-windows-helper.sh <release-directory>}
DEST="$OUT/tools/windows-network-panel"
mkdir -p "$DEST"
for file in NetworkPanel.ps1 TrafficCollector.ps1 HotspotBridge.ps1 Start-Dashboard.ps1 Start-Dashboard.cmd Start-NetworkPanel.cmd README.md; do
    cp "$ROOT/tools/windows-network-panel/$file" "$DEST/$file"
done
cp "$ROOT/tools/windows-network-panel/Start-PackagedDashboard.cmd" "$OUT/Start-Dashboard.cmd"
