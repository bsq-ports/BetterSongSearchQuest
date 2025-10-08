#!/usr/bin/env pwsh
$NDKPath = Get-Content ./ndkpath.txt

$stackScript = "$NDKPath/ndk-stack"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $stackScript += ".cmd"
}

Get-Content ./logcat.log | & $stackScript -sym ./build/debug/ > log_unstripped.log
