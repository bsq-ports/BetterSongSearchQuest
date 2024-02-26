# Windows version
param($p1)

if (Test-Path "./ndkpath.txt")
{
    $NDKPath = Get-Content ./ndkpath.txt
} else {
    $NDKPath = $ENV:ANDROID_NDK_HOME
}

if ($p1)
{
    & $NDKPath\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-addr2line.exe -e .\build\debug\libBetterSongSearch.so $p1
}
else
{
	echo give at least 1 argument
}
