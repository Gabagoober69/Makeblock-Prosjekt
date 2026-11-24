function ConvertFrom-ChannelLine {
    param([string]$Line)
    if ($Line -notmatch '^(OK|LOST)\s+(.+?)\s*$') { return $null }
    $state = $Matches[1]
    $tokens = $Matches[2] -split '\s+'
    if ($tokens.Count -ne 16) { return $null }
    $values = New-Object int[] 16
    for ($i = 0; $i -lt 16; $i++) {
        if ($tokens[$i] -notmatch '^CH(\d+)=(\d{1,4})$') { return $null }
        if ([int]$Matches[1] -ne ($i+1) -or [int]$Matches[2] -gt 2047) { return $null }
        $values[$i] = [int]$Matches[2]
    }
    [pscustomobject]@{ State = $state; Values = $values }
}

function Get-ChannelPercent {
    param([int]$Raw)
    # Standard CRSF endpoints; display clamps extended limits to +/-100%.
    $span = if ($Raw -ge 992) { 819.0 } else { 820.0 }
    [int][Math]::Round([Math]::Max(-100, [Math]::Min(100, ($Raw-992)*100.0/$span)))
}
Export-ModuleMember -Function ConvertFrom-ChannelLine, Get-ChannelPercent
