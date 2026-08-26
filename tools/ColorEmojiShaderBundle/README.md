# Color emoji shader bundle

This Unity 6000.0.40f1 project builds the Android `TextMeshPro/Sprite` shader bundle embedded by BetterSongSearch.

```powershell
unity run . --editor-version 6000.0.40f1 -- -nographics -quit -executeMethod BuildColorEmojiShaderBundle.BuildAndroid
```

Copy `Build/Android/bss-color-emoji-shader.bundle` to the repository's `assets` directory after rebuilding it.

`BuildColorEmojiShaderBundle.ValidateQuestColorFont` is an optional local check for a pulled Quest emoji font. Import Unity's TMP Essential Resources first, then pass the font with `-emojiFontPath`. The imported resources are intentionally ignored.
