#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
OUT=${1:?Usage: package-windows-helper.sh <release-directory>}
DEST="$OUT/tools/windows-network-panel"
mkdir -p "$DEST"
DEST=$(cd -- "$DEST" && pwd)
for file in NetworkPanel.ps1 TrafficCollector.ps1 HotspotBridge.ps1 Start-Dashboard.ps1 Start-Dashboard.cmd Start-NetworkPanel.cmd Get-ValidationEnvironment.ps1 HARDWARE-VALIDATION.md README.md; do
    if [ "$ROOT/tools/windows-network-panel" != "$DEST" ]; then
        cp "$ROOT/tools/windows-network-panel/$file" "$DEST/$file"
    fi
done
cp "$ROOT/tools/windows-network-panel/Start-PackagedDashboard.cmd" "$OUT/Start-Dashboard.cmd"
