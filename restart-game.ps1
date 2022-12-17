adb shell am force-stop com.beatgames.beatsaber
adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity
# Workaround for Meta piracy notification
# Start-Sleep -Seconds 1.0
# adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity