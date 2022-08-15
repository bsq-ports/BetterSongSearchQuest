#!/bin/bash

buildScript="$ANDROID_NDK_HOME/build/ndk-build"

$buildScript NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk -j32
adb push libs/arm64-v8a/libcustom-json-data.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libcustom-json-data.so
adb shell am force-stop com.beatgames.beatsaber
adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity

adb logcat -c && adb logcat > test.log
