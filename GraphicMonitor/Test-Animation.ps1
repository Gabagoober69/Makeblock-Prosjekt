$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -Path (Join-Path $PSScriptRoot 'ChannelBar.cs') -ReferencedAssemblies System.Windows.Forms,System.Drawing
$bar=New-Object OrionChannelBar
try {
    $bar.Percent=0; $bar.Live=$true; $bar.Percent=100
    $bar.AdvanceAnimation(0.016)
    if ($bar.DisplayedPercent -le 0 -or $bar.DisplayedPercent -ge 100) { throw 'Missing interpolation' }
    $bar.AdvanceAnimation(0.119)
    if ([Math]::Abs($bar.DisplayedPercent-95.0213) -gt 0.01) { throw 'Time constant incorrect' }
    $bar.SnapToTarget(); $bar.Percent=-100
    $previous=$bar.DisplayedPercent
    for ($i=0; $i -lt 40; $i++) {
        $bar.AdvanceAnimation(0.016)
        if ($bar.DisplayedPercent -gt $previous -or $bar.DisplayedPercent -lt -100) { throw 'Overshoot' }
        $previous=$bar.DisplayedPercent
    }
    if ($bar.DisplayedPercent -ne -100) { throw 'Did not settle' }
    $bar.Percent=70; $bar.AdvanceAnimation(0.016); $bar.Live=$false
    if ($bar.DisplayedPercent -ne 70) { throw 'Stale display must show last measured value' }
    $bar.AdvanceAnimation(1)
    if ($bar.DisplayedPercent -ne 70) { throw 'Stale display moved' }
    $bar.Percent=999
    if ($bar.Percent -ne 100 -or $bar.DisplayedPercent -ne 100) { throw 'Clamp failed' }
    'PASS: interpolation, time constant, no overshoot, settling, stale freeze, clamping.'
} finally { $bar.Dispose() }
