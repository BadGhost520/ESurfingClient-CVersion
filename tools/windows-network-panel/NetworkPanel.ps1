# Requires Windows PowerShell 5.1 (WinRT projection is unavailable in PowerShell 7).
[CmdletBinding()]
param([switch]$LibraryOnly, [switch]$Check, [string]$PreviewPath)

$ErrorActionPreference = 'Stop'
trap {
    if ($LibraryOnly -or $Check) { throw $_ }
    Add-Type -AssemblyName System.Windows.Forms
    [void][Windows.Forms.MessageBox]::Show($_.Exception.GetBaseException().Message, '网络面板无法启动')
    exit 1
}

function Get-TrafficRate($Previous, $Current) {
    if ($null -eq $Previous -or $null -eq $Current) { return $null }
    $elapsed = $Current.Time - $Previous.Time
    if ($elapsed -le 0 -or $Current.Received -lt $Previous.Received -or
        $Current.Sent -lt $Previous.Sent) { return $null }
    return @{ Down = ($Current.Received - $Previous.Received) / $elapsed
              Up = ($Current.Sent - $Previous.Sent) / $elapsed }
}

function Format-Rate([double]$Bytes) {
    $culture = [Globalization.CultureInfo]::InvariantCulture
    if ($Bytes -ge 1MB) { return ($Bytes / 1MB).ToString('0.00', $culture) + ' MiB/s' }
    if ($Bytes -ge 1KB) { return ($Bytes / 1KB).ToString('0.00', $culture) + ' KiB/s' }
    return $Bytes.ToString('0', $culture) + ' B/s'
}

if ($LibraryOnly) { return }
if ($PSVersionTable.PSEdition -ne 'Desktop') { throw '请用 Start-NetworkPanel.cmd 启动（需要 Windows PowerShell 5.1）。' }

Add-Type -AssemblyName System.Windows.Forms, System.Drawing, System.Runtime.WindowsRuntime
[Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime] > $null
[Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime] > $null
[Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime] > $null
[System.Windows.Forms.Application]::EnableVisualStyles()

$script:clock = [Diagnostics.Stopwatch]::StartNew()
$script:previous = $null
$script:manager = $null
$script:pending = $null
$script:adapters = @()
$script:profiles = @{}
$script:state = 'Unknown'
$script:capability = ''
$script:refreshAt = -10.0
$script:loading = $false
# The native async operation remains asynchronous; the UI timer observes completion.
$script:asTask = [System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
    $_.Name -eq 'AsTask' -and $_.IsGenericMethod -and
    $_.GetGenericArguments().Count -eq 1 -and $_.GetParameters().Count -eq 1 -and
    $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'
} | Select-Object -First 1
$script:tetheringTask = $script:asTask.MakeGenericMethod([Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult])

function Add-Label($Text, $X, $Y, $Width, $Height) {
    $label = New-Object Windows.Forms.Label
    $label.Text = $Text
    $label.SetBounds($X, $Y, $Width, $Height)
    $form.Controls.Add($label)
    return $label
}

$form = New-Object Windows.Forms.Form
$form.Text = 'ESurfing · 网速与手机热点'
$form.ClientSize = New-Object Drawing.Size(640, 520)
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.StartPosition = 'CenterScreen'
$form.AutoScaleMode = 'Dpi'
$form.Font = New-Object Drawing.Font('Microsoft YaHei UI', 10)
$form.BackColor = [Drawing.Color]::White
$heading = Add-Label '网速与手机热点' 24 22 580 36
$heading.Font = New-Object Drawing.Font('Microsoft YaHei UI', 18, ([Drawing.FontStyle]::Bold))
$null = Add-Label '独立运行，校园网认证继续由原客户端负责。' 26 66 580 26
$null = Add-Label '校园网使用的上联网卡' 26 105 420 26
$select = New-Object Windows.Forms.ComboBox
$select.DropDownStyle = 'DropDownList'
$select.SetBounds(26, 135, 450, 32)
$form.Controls.Add($select)
$reload = New-Object Windows.Forms.Button
$reload.Text = '刷新网卡'
$reload.SetBounds(490, 133, 124, 34)
$form.Controls.Add($reload)
$down = Add-Label '下载  —' 26 185 290 42
$up = Add-Label '上传  —' 324 185 290 42
foreach ($label in @($down, $up)) { $label.Font = New-Object Drawing.Font('Microsoft YaHei UI', 18, ([Drawing.FontStyle]::Bold)) }
$trafficHint = Add-Label '每秒采样；统计此网卡全部流量，不是测速或套餐带宽。' 26 235 588 42
$hotspot = Add-Label '手机热点：读取中' 26 288 588 28
$ssid = Add-Label '热点名称：—' 26 320 588 28
$start = New-Object Windows.Forms.Button
$start.Text = '开启热点'
$start.SetBounds(26, 359, 130, 38)
$form.Controls.Add($start)
$stop = New-Object Windows.Forms.Button
$stop.Text = '关闭热点'
$stop.SetBounds(168, 359, 130, 38)
$form.Controls.Add($stop)
$settings = New-Object Windows.Forms.Button
$settings.Text = '设置热点名称 / 密码'
$settings.SetBounds(310, 359, 304, 38)
$form.Controls.Add($settings)
$message = Add-Label '启动面板不会开关热点。关闭窗口也不会改变热点状态。' 26 414 588 48
$message.ForeColor = [Drawing.Color]::FromArgb(55, 65, 81)
$null = Add-Label '首次开启可能影响 Wi-Fi 连接；建议先使用有线校园网。' 26 475 588 28

function Update-HotspotState {
    $start.Enabled = $false
    $stop.Enabled = $false
    $script:state = 'Unknown'
    try {
        if ($null -eq $script:manager) { throw '当前网卡没有可用的 Windows 网络配置。' }
        $script:state = [string]$script:manager.TetheringOperationalState
        $names = @{ Off = '已关闭'; On = '已开启'; InTransition = '正在切换'; Unknown = '未知' }
        $hotspot.Text = '手机热点：' + $names[$script:state]
        $ssid.Text = '热点名称：' + $script:manager.GetCurrentAccessPointConfiguration().Ssid
        if ($script:capability -ne 'Enabled' -and $script:state -eq 'Off') {
            $hotspot.Text = '手机热点：不可用（' + $script:capability + '）'
        }
        $start.Enabled = ($script:state -eq 'Off' -and $script:capability -eq 'Enabled' -and $null -eq $script:pending)
        $stop.Enabled = ($script:state -eq 'On' -and $null -eq $script:pending)
    } catch {
        $hotspot.Text = '手机热点：无法读取状态'
        $ssid.Text = '可打开系统设置检查无线网卡和热点支持。'
    }
    # Do not change uplink while sharing or while Windows is transitioning.
    $select.Enabled = ($null -eq $script:pending -and $script:state -notin @('On', 'InTransition'))
    $reload.Enabled = $select.Enabled
}

function Select-Uplink {
    $script:previous = $null
    $script:manager = $null
    $script:capability = ''
    $down.Text = '下载  —'
    $up.Text = '上传  —'
    if ($select.SelectedIndex -ge 0) {
        $adapter = $script:adapters[$select.SelectedIndex]
        $profile = $script:profiles[$adapter.Id.Trim('{}').ToLowerInvariant()]
        try {
            $script:capability = [string][Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::GetTetheringCapabilityFromConnectionProfile($profile)
            $script:manager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager]::CreateFromConnectionProfile($profile)
        } catch { $message.Text = '热点暂不可用。网速统计仍可使用；请在系统设置检查热点。' }
    }
    Update-HotspotState
}

function Reload-Uplinks {
    $script:loading = $true
    try {
        $oldId = if ($select.SelectedIndex -ge 0) { $script:adapters[$select.SelectedIndex].Id } else { '' }
        $defaultProfile = [Windows.Networking.Connectivity.NetworkInformation]::GetInternetConnectionProfile()
        $defaultId = if ($null -ne $defaultProfile -and $null -ne $defaultProfile.NetworkAdapter) {
            $defaultProfile.NetworkAdapter.NetworkAdapterId.ToString()
        } else { '' }
        $script:profiles = @{}
        foreach ($profile in [Windows.Networking.Connectivity.NetworkInformation]::GetConnectionProfiles()) {
            if ($null -ne $profile.NetworkAdapter) {
                $script:profiles[$profile.NetworkAdapter.NetworkAdapterId.ToString().ToLowerInvariant()] = $profile
            }
        }
        # Match Windows profiles to avoid counting driver/filter/loopback interfaces twice.
        $script:adapters = @([Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces() | Where-Object {
            $_.OperationalStatus -eq 'Up' -and $script:profiles.ContainsKey($_.Id.Trim('{}').ToLowerInvariant())
        })
        $select.Items.Clear()
        $index = 0
        for ($i = 0; $i -lt $script:adapters.Count; $i++) {
            $adapter = $script:adapters[$i]
            [void]$select.Items.Add($adapter.Name + ' — ' + $adapter.Description)
            if ($adapter.Id -eq $oldId -or (-not $oldId -and $adapter.Id.Trim('{}') -eq $defaultId)) { $index = $i }
        }
        if ($select.Items.Count -gt 0) { $select.SelectedIndex = $index }
        else { $message.Text = '未找到已连接的上联网卡。连接校园网后点击“刷新网卡”。' }
    } finally { $script:loading = $false }
    Select-Uplink
}

function Request-Hotspot([bool]$Enable) {
    if ($null -ne $script:pending -or $null -eq $script:manager) { return }
    Update-HotspotState
    if (($Enable -and -not $start.Enabled) -or (-not $Enable -and -not $stop.Enabled)) { return }
    try {
        # These are the ONLY mutating network calls. They run only on button clicks.
        $operation = if ($Enable) { $script:manager.StartTetheringAsync() } else { $script:manager.StopTetheringAsync() }
        $script:pending = $script:tetheringTask.Invoke($null, @($operation))
        $message.Text = 'Windows 正在处理，请稍候。校园网认证进程继续运行。'
    } catch { $message.Text = '操作未成功：' + $_.Exception.GetBaseException().Message }
    Update-HotspotState
}

function Update-Panel {
    if ($null -ne $script:pending -and $script:pending.IsCompleted) {
        try {
            $result = $script:pending.GetAwaiter().GetResult()
            $message.Text = if ([string]$result.Status -eq 'Success') { '操作成功。手机可使用系统设置里的热点名称和密码连接。' }
                else { 'Windows 未完成操作：' + [string]$result.Status + '。请打开系统设置检查。' }
        } catch { $message.Text = '操作未成功：' + $_.Exception.GetBaseException().Message }
        $script:pending = $null
        Update-HotspotState
    }
    if ($script:clock.Elapsed.TotalSeconds - $script:refreshAt -ge 5) {
        Update-HotspotState
        $script:refreshAt = $script:clock.Elapsed.TotalSeconds
    }
    if ($select.SelectedIndex -lt 0) { return }
    try {
        $id = $script:adapters[$select.SelectedIndex].Id
        $adapter = [Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces() | Where-Object { $_.Id -eq $id } | Select-Object -First 1
        if ($null -eq $adapter -or $adapter.OperationalStatus -ne 'Up') { throw '所选网卡已断开，请连接后刷新网卡。' }
        $stats = $adapter.GetIPStatistics()
        $sample = @{ Received = $stats.BytesReceived; Sent = $stats.BytesSent; Time = $script:clock.Elapsed.TotalSeconds }
        $rate = Get-TrafficRate $script:previous $sample
        $script:previous = $sample
        $down.Text = if ($null -eq $rate) { '下载  采样中' } else { '下载  ' + (Format-Rate $rate.Down) }
        $up.Text = if ($null -eq $rate) { '上传  采样中' } else { '上传  ' + (Format-Rate $rate.Up) }
        $trafficHint.Text = '每秒采样；统计此网卡全部流量，不是测速或套餐带宽。'
    } catch {
        $script:previous = $null
        $down.Text = '下载  —'
        $up.Text = '上传  —'
        $trafficHint.Text = $_.Exception.GetBaseException().Message
    }
}

$select.Add_SelectedIndexChanged({ if (-not $script:loading) { Select-Uplink } })
$reload.Add_Click({ try { Reload-Uplinks } catch { $message.Text = '刷新失败：' + $_.Exception.GetBaseException().Message } })
$start.Add_Click({ Request-Hotspot $true })
$stop.Add_Click({ Request-Hotspot $false })
$settings.Add_Click({ try { Start-Process 'ms-settings:network-mobilehotspot' } catch { $message.Text = '请在 Windows 设置 → 网络和 Internet → 移动热点中配置。' } })
$timer = New-Object Windows.Forms.Timer
$timer.Interval = 1000
$timer.Add_Tick({ Update-Panel })
try {
    Reload-Uplinks
    Update-Panel
    if ($Check) {
        Start-Sleep -Milliseconds 1100
        Update-Panel
        if ($PreviewPath) {
            $form.Opacity = 0
            $form.Show()
            [Windows.Forms.Application]::DoEvents()
            $bitmap = New-Object Drawing.Bitmap($form.Width, $form.Height)
            try { $form.DrawToBitmap($bitmap, (New-Object Drawing.Rectangle(0, 0, $form.Width, $form.Height))); $bitmap.Save($PreviewPath) }
            finally { $bitmap.Dispose() }
        }
        [pscustomobject]@{ Adapters = $script:adapters.Count; HotspotState = $script:state
            Capability = $script:capability; Download = $down.Text; Upload = $up.Text
            CanStart = $start.Enabled; CanStop = $stop.Enabled; Pending = ($null -ne $script:pending) }
    } else {
        $timer.Start()
        [void]$form.ShowDialog()
    }
} finally {
    # Closing this utility never stops authentication or toggles Windows hotspot.
    $timer.Stop()
    $timer.Dispose()
    $form.Dispose()
}
