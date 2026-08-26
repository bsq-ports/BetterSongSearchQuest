using System;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.TextCore.LowLevel;
using TMPro;

public static class BuildColorEmojiShaderBundle
{
    private const string ShaderPath = "Assets/TextMeshProSprite.shader";
    private const string BundleName = "bss-color-emoji-shader.bundle";

    public static void BuildAndroid()
    {
        var shader = AssetDatabase.LoadAssetAtPath<Shader>(ShaderPath);
        if (shader == null || shader.name != "TextMeshPro/Sprite")
        {
            throw new InvalidOperationException($"Unable to load {ShaderPath} as TextMeshPro/Sprite");
        }

        PlayerSettings.SetGraphicsAPIs(BuildTarget.Android, new[] { GraphicsDeviceType.OpenGLES3 });

        var outputDirectory = Path.GetFullPath(Path.Combine(Application.dataPath, "../Build/Android"));
        Directory.CreateDirectory(outputDirectory);

        var build = new AssetBundleBuild
        {
            assetBundleName = BundleName,
            assetNames = new[] { ShaderPath },
            addressableNames = new[] { "tmp_sprite.shader" }
        };

        var manifest = BuildPipeline.BuildAssetBundles(
            outputDirectory,
            new[] { build },
            BuildAssetBundleOptions.ChunkBasedCompression |
                BuildAssetBundleOptions.StrictMode |
                BuildAssetBundleOptions.ForceRebuildAssetBundle,
            BuildTarget.Android
        );

        var bundlePath = Path.Combine(outputDirectory, BundleName);
        if (manifest == null || !File.Exists(bundlePath))
        {
            throw new InvalidOperationException("Unity did not produce the color emoji shader bundle");
        }

        Debug.Log($"Built {bundlePath} ({new FileInfo(bundlePath).Length} bytes)");
    }

    public static void ValidateQuestColorFont()
    {
        AssetDatabase.LoadAssetAtPath<Shader>(ShaderPath);

        var arguments = Environment.GetCommandLineArgs();
        var pathArgument = Array.IndexOf(arguments, "-emojiFontPath");
        if (pathArgument < 0 || pathArgument + 1 >= arguments.Length)
        {
            throw new ArgumentException("Pass -emojiFontPath followed by the font path");
        }

        var font = TMP_FontAsset.CreateFontAsset(
            arguments[pathArgument + 1],
            0,
            55,
            0,
            GlyphRenderMode.COLOR,
            2048,
            2048
        );
        if (font == null)
        {
            throw new InvalidOperationException("TMP failed to create the Quest color font asset");
        }

        var emojiAdded = font.TryAddCharacters(new uint[] { 0x1F600, 0x1F308 }, false);
        Debug.Log($"Quest color font validation: emojiAdded={emojiAdded}, atlas={font.atlasTexture.width}x{font.atlasTexture.height}/{font.atlasTexture.format}");

        if (!emojiAdded)
        {
            throw new InvalidOperationException("TMP could not rasterize the Quest color emoji test glyphs");
        }
    }
}
