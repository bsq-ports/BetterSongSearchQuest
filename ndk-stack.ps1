$NDKPath = Get-Content ./ndkpath.txt

$stackScript = "$NDKPath/ndk-stack"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $stackScript += ".cmd"
}

if ($args.Count -eq 0) {
    Get-Content ./log.txt | & $stackScript -sym ./obj/local/arm64-v8a/ > log_unstripped.log
} else {
    Get-Content $args[0] | & $stackScript -sym ./obj/local/arm64-v8a/ > "$($args[0])_unstripped.txt"
}
