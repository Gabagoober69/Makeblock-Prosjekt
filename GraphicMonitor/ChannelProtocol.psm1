function ConvertFrom-ChannelLine {
    # Reads one text line from ChannelMonitor, for example:
    #   OK CH2=992 CH4=992 CH5=191 CH6=191
    # Returns State (OK or LOST) and Values, a hashtable: channel number -> raw value.
    # Returns $null if the line is malformed or a required channel is missing.
    param([string]$Line, [int[]]$Required = @())
    if ($Line -notmatch '^(OK|LOST)\s+(.+?)\s*$') { return $null }
    $state = $Matches[1]
    $tokens = $Matches[2] -split '\s+'
    $values = @{}
    foreach ($token in $tokens) {
        if ($token -notmatch '^CH(\d{1,2})=(\d{1,4})$') { return $null }
        $ch = [int]$Matches[1]; $raw = [int]$Matches[2]
        if ($ch -lt 1 -or $ch -gt 16 -or $raw -gt 2047) { return $null }
        $values[$ch] = $raw
    }
    foreach ($ch in $Required) { if (-not $values.ContainsKey($ch)) { return $null } }
    [pscustomobject]@{ State = $state; Values = $values }
}

function Get-ChannelPercent {
    param([int]$Raw)
    # Standard CRSF endpoints; display clamps extended limits to +/-100%.
    $span = if ($Raw -ge 992) { 819.0 } else { 820.0 }
    [int][Math]::Round([Math]::Max(-100, [Math]::Min(100, ($Raw-992)*100.0/$span)))
}
Export-ModuleMember -Function ConvertFrom-ChannelLine, Get-ChannelPercent
