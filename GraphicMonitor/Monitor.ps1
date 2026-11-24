param([string]$PreviewPath)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -Path (Join-Path $PSScriptRoot 'ChannelBar.cs') -ReferencedAssemblies System.Windows.Forms,System.Drawing
Import-Module (Join-Path $PSScriptRoot 'ChannelProtocol.psm1') -Force
[System.Windows.Forms.Application]::EnableVisualStyles()

# ---- Channels used by RobotControl (must match the settings in RobotControl.ino) ----
$channels = @(
    [pscustomobject]@{ Ch=2; Role='GASS';     Hint='hoyre spak opp/ned';     Kind='stick'; Color='#38BDF8' },
    [pscustomobject]@{ Ch=4; Role='STYRING';  Hint='venstre spak sideveis';  Kind='stick'; Color='#C084FC' },
    [pscustomobject]@{ Ch=5; Role='SA';       Hint='hovedbryter';            Kind='arm';   Color='#34D399' },
    [pscustomobject]@{ Ch=6; Role='SF';       Hint='robotlyd';               Kind='sound'; Color='#FBBF24' }
)
$required = [int[]]($channels | ForEach-Object { $_.Ch })
$SwitchHigh   = 1500    # raw >= this = switch high (RobotControl.ino SWITCH_HIGH)
$SwitchLow    = 484     # raw <= this = switch low  (RobotControl.ino SWITCH_LOW)
$SaDownIsHigh = $false  # measured on this radio: SA down = 191 (low)

$script:serial = $null
$script:buffer = ''
$script:lastFrame = [datetime]::MinValue
$script:linkState = 'LOST'
$script:demo = $false
$script:demoStart = Get-Date
$script:bars = @()
$script:values = @()

$form = New-Object System.Windows.Forms.Form
$form.Text = 'Orion ELRS | kanalmonitor'
$form.Size = New-Object System.Drawing.Size(1000, 720)
$form.MinimumSize = New-Object System.Drawing.Size(800, 600)
$form.StartPosition = 'CenterScreen'
$form.BackColor = [System.Drawing.Color]::FromArgb(20,26,38)
$form.ForeColor = [System.Drawing.Color]::WhiteSmoke
$form.Font = New-Object System.Drawing.Font('Segoe UI', 10)

$layout = New-Object System.Windows.Forms.TableLayoutPanel
$layout.Dock = 'Fill'
$layout.Padding = New-Object System.Windows.Forms.Padding(16)
$layout.ColumnCount = 1
$layout.RowCount = 5
foreach ($height in @(45,45,45,0,55)) {
    $style = New-Object System.Windows.Forms.RowStyle
    if ($height) { $style.SizeType='Absolute'; $style.Height=$height }
    else { $style.SizeType='Percent'; $style.Height=100 }
    [void]$layout.RowStyles.Add($style)
}
$form.Controls.Add($layout)
$title = New-Object System.Windows.Forms.Label
$title.Text = 'ELRS / KANALMONITOR'
$title.Font = New-Object System.Drawing.Font('Segoe UI', 19, [System.Drawing.FontStyle]::Bold)
$title.Dock = 'Fill'
$layout.Controls.Add($title,0,0)

$toolbar = New-Object System.Windows.Forms.FlowLayoutPanel
$toolbar.Dock = 'Fill'
$portBox = New-Object System.Windows.Forms.ComboBox
$portBox.DropDownStyle='DropDownList'; $portBox.Width=120
$refresh = New-Object System.Windows.Forms.Button
$refresh.Text='Oppdater'; $refresh.Width=100
$connect = New-Object System.Windows.Forms.Button
$connect.Text='Koble til'; $connect.Width=120
$demoButton = New-Object System.Windows.Forms.Button
$demoButton.Text='Vis demo'; $demoButton.Width=110
foreach ($button in @($refresh,$connect,$demoButton)) {
    $button.ForeColor=[System.Drawing.Color]::FromArgb(15,23,42)
    $button.BackColor=[System.Drawing.Color]::FromArgb(203,224,245)
    $button.UseVisualStyleBackColor=$false
    $button.FlatStyle='Flat'
}
$baud = New-Object System.Windows.Forms.Label
$baud.Text='115200 baud / 8N1'; $baud.AutoSize=$true; $baud.Padding=New-Object System.Windows.Forms.Padding(10,7,0,0)
$toolbar.Controls.AddRange(@($portBox,$refresh,$connect,$demoButton,$baud))
$layout.Controls.Add($toolbar,0,1)
$status = New-Object System.Windows.Forms.Label
$status.Text='FRAKOBLET - velg COM-port. RobotControl maa vaere lastet opp til Orion.'
$status.Dock='Fill'; $status.TextAlign='MiddleLeft'
$layout.Controls.Add($status,0,2)

# One card per used channel, side by side.
$grid = New-Object System.Windows.Forms.TableLayoutPanel
$grid.Dock='Fill'; $grid.ColumnCount=$channels.Count; $grid.RowCount=1
for ($i=0; $i -lt $channels.Count; $i++) {
    [void]$grid.ColumnStyles.Add((New-Object System.Windows.Forms.ColumnStyle('Percent',(100.0/$channels.Count))))
}
[void]$grid.RowStyles.Add((New-Object System.Windows.Forms.RowStyle('Percent',100)))
for ($i=0; $i -lt $channels.Count; $i++) {
    $c = $channels[$i]
    $accent=[System.Drawing.ColorTranslator]::FromHtml($c.Color)
    $card = New-Object System.Windows.Forms.Panel
    $card.Dock='Fill'; $card.Margin=New-Object System.Windows.Forms.Padding(6)
    $card.BackColor=[System.Drawing.Color]::FromArgb(32,42,58)
    $label = New-Object System.Windows.Forms.Label
    $label.Text=('CH{0} {1}' -f $c.Ch,$c.Role); $label.Dock='Top'; $label.Height=32
    $label.TextAlign='MiddleCenter'; $label.ForeColor=$accent
    $label.Font=New-Object System.Drawing.Font('Segoe UI',14,[System.Drawing.FontStyle]::Bold)
    $role = New-Object System.Windows.Forms.Label
    $role.Text=$c.Hint; $role.Dock='Top'; $role.Height=24; $role.TextAlign='MiddleCenter'
    $value = New-Object System.Windows.Forms.Label
    $value.Text='--'; $value.Dock='Bottom'; $value.Height=52; $value.TextAlign='MiddleCenter'
    $value.Font=New-Object System.Drawing.Font('Consolas',12)
    $bar = New-Object OrionChannelBar
    $bar.Dock='Fill'; $bar.Accent=$accent
    $card.Controls.AddRange(@($bar,$value,$role,$label))
    $grid.Controls.Add($card,$i,0)
    $script:bars += $bar; $script:values += $value
}
$layout.Controls.Add($grid,0,3)
$footer = New-Object System.Windows.Forms.Label
$footer.Text="Stolper: -100% nederst / 0% ved midtlinjen / +100% overst. CRSF: ca. 172 / 992 / 1811.`r`nKun visning. Graa stolper = gamle verdier. Roboten er bare aktiv med SA NED og radiokontakt."
$footer.Dock='Fill'; $footer.TextAlign='MiddleLeft'
$layout.Controls.Add($footer,0,4)

# Plain-language state for switch channels.
function Get-StateText([string]$Kind, [int]$Raw) {
    $high = $Raw -ge $SwitchHigh
    $low  = $Raw -le $SwitchLow
    if ($Kind -eq 'arm') {
        $down = if ($SaDownIsHigh) { $high } else { $low }
        $up   = if ($SaDownIsHigh) { $low }  else { $high }
        if ($down) { return 'NED = AKTIV' }
        if ($up)   { return 'OPP = AV' }
        return 'UKJENT'
    }
    if ($Kind -eq 'sound') { if ($high) { return 'PAA' } else { return 'AV' } }
    return ''
}

function Update-Ports {
    $selected=$portBox.Text
    $portBox.Items.Clear()
    foreach ($name in ([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)) { [void]$portBox.Items.Add($name) }
    if ($portBox.Items.Contains($selected)) { $portBox.SelectedItem=$selected }
    elseif ($portBox.Items.Count) { $portBox.SelectedIndex=0 }
}
function Close-Port {
    if ($null -ne $script:serial) {
        try { $script:serial.Close(); $script:serial.Dispose() } catch {}
        $script:serial=$null
    }
    $script:buffer=''; $script:lastFrame=[datetime]::MinValue
    $connect.Text='Koble til'; $portBox.Enabled=$true; $refresh.Enabled=$true
}
function Show-Values($frame) {
    for ($i=0; $i -lt $channels.Count; $i++) {
        $c=$channels[$i]
        $raw=[int]$frame.Values[$c.Ch]
        $pct=Get-ChannelPercent $raw
        $script:bars[$i].Percent=$pct
        if ($c.Kind -eq 'stick') {
            $script:values[$i].Text=("{0:+0;-0;0}%`r`nRAW {1}" -f $pct,$raw)
        } else {
            $script:values[$i].Text=("{0}`r`nRAW {1}" -f (Get-StateText $c.Kind $raw),$raw)
        }
    }
}
$refresh.Add_Click({ Update-Ports })
$connect.Add_Click({
    if ($null -ne $script:serial) { Close-Port; $status.Text='FRAKOBLET'; return }
    if (-not $portBox.Text) { $status.Text='Ingen COM-port valgt. Koble til USB og trykk Oppdater.'; return }
    $script:demo=$false; $demoButton.Text='Vis demo'
    try {
        $script:serial=New-Object System.IO.Ports.SerialPort($portBox.Text,115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
        $script:serial.Handshake=[System.IO.Ports.Handshake]::None
        $script:serial.DtrEnable=$false; $script:serial.RtsEnable=$false
        $script:serial.Open(); $script:serial.DiscardInBuffer()
        $script:buffer=''; $script:lastFrame=[datetime]::MinValue
        $connect.Text='Koble fra'; $portBox.Enabled=$false; $refresh.Enabled=$false
        $status.Text='VENTER - koblet til, venter paa data fra RobotControl.'
    } catch {
        $detail=$_.Exception.Message; Close-Port
        $status.Text="PORTFEIL: $detail Lukk Arduino Serial Monitor og andre terminaler."
    }
})
$demoButton.Add_Click({
    Close-Port
    $script:demo=-not $script:demo
    $script:demoStart=Get-Date
    $demoButton.Text=if ($script:demo) { 'Stopp demo' } else { 'Vis demo' }
    $status.Text=if ($script:demo) { 'DEMO - simulerte verdier, ingen radio tilkoblet.' } else { 'FRAKOBLET' }
})
$timer=New-Object System.Windows.Forms.Timer
$timer.Interval=16
$animationClock=[System.Diagnostics.Stopwatch]::StartNew()
$timer.Add_Tick({
    $elapsed=$animationClock.Elapsed.TotalSeconds
    $animationClock.Restart()
    foreach ($b in $script:bars) { $b.AdvanceAnimation($elapsed) }
    if ($script:demo) {
        $t=((Get-Date)-$script:demoStart).TotalSeconds
        $data=@{}
        for ($i=0; $i -lt $channels.Count; $i++) {
            $c=$channels[$i]
            if ($c.Kind -eq 'stick') { $data[$c.Ch]=[int](992+780*[Math]::Sin($t*0.8+$i*1.3)) }
            else { $data[$c.Ch]=if ([Math]::Floor($t/3+$i) % 2) { 1792 } else { 191 } }
        }
        Show-Values ([pscustomobject]@{ Values=$data })
        $status.ForeColor=[System.Drawing.Color]::Gold
        foreach ($v in $script:values) { $v.ForeColor=[System.Drawing.Color]::Gold }
        foreach ($b in $script:bars) { $b.Live=$true }
        return
    }
    if ($null -ne $script:serial) {
        try {
            $script:buffer += $script:serial.ReadExisting()
            $lines=$script:buffer -split "`n"
            $script:buffer=$lines[-1]
            $latest=$null
            for ($i=0; $i -lt $lines.Count-1; $i++) {
                $parsed=ConvertFrom-ChannelLine $lines[$i].TrimEnd("`r") $required
                if ($null -ne $parsed) { $latest=$parsed }
            }
            if ($script:buffer.Length -gt 8192) { $script:buffer='' }
            if ($null -ne $latest) {
                $script:lastFrame=Get-Date; $script:linkState=$latest.State
                Show-Values $latest
            }
            $age=((Get-Date)-$script:lastFrame).TotalMilliseconds
            $live=$age -lt 1000 -and $script:linkState -eq 'OK'
            if ($live) {
                $status.Text="LIVE - $($script:serial.PortName) | kanaldata opptil 20 ganger/s"
                $status.ForeColor=[System.Drawing.Color]::LightGreen
            } else {
                $status.Text='INGEN GYLDIG RADIOINPUT - gamle verdier vises i graatt. Kontroller radio, mottaker og at RobotControl er lastet opp.'
                $status.ForeColor=[System.Drawing.Color]::Orange
            }
            foreach ($v in $script:values) { $v.ForeColor=if ($live) { [System.Drawing.Color]::WhiteSmoke } else { [System.Drawing.Color]::Gray } }
            foreach ($b in $script:bars) { $b.Live=$live }
        } catch {
            $detail=$_.Exception.Message; Close-Port
            $status.Text="FORBINDELSEN BRUTT: $detail"
            foreach ($b in $script:bars) { $b.Live=$false }
        }
    } else {
        $status.ForeColor=[System.Drawing.Color]::Silver
        foreach ($v in $script:values) { $v.ForeColor=[System.Drawing.Color]::Gray }
        foreach ($b in $script:bars) { $b.Live=$false }
    }
})
$form.Add_FormClosed({ $timer.Stop(); Close-Port; $timer.Dispose() })
Update-Ports
if ($PreviewPath) {
    $status.Text='DEMO - simulerte verdier, ingen radio tilkoblet.'
    $status.ForeColor=[System.Drawing.Color]::Gold
    Show-Values ([pscustomobject]@{ Values=@{ 2=1500; 4=700; 5=191; 6=191 } })
    foreach ($b in $script:bars) { $b.Live=$true }
    [void]$form.Handle
    $form.PerformLayout(); $layout.PerformLayout(); $grid.PerformLayout()
    foreach ($card in $grid.Controls) { $card.PerformLayout() }
    function Initialize-PreviewControls($control) {
        [void]$control.Handle
        $control.PerformLayout()
        foreach ($child in $control.Controls) { Initialize-PreviewControls $child }
    }
    Initialize-PreviewControls $form
    $bitmap=New-Object System.Drawing.Bitmap($form.Width,$form.Height)
    try {
        $form.DrawToBitmap($bitmap,(New-Object System.Drawing.Rectangle(0,0,$form.Width,$form.Height)))
        $bitmap.Save($PreviewPath)
    } finally { $bitmap.Dispose(); $timer.Dispose() }
} else {
    $timer.Start()
    [void]$form.ShowDialog()
}
$form.Dispose()
