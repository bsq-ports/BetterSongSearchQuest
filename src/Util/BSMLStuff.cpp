#include "Util/BSMLStuff.hpp"

#include <filesystem>
#include <fstream>

#include "assets.hpp"
#include "bsml/shared/Helpers/getters.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/CustomFormatRangeValuesSlider.hpp"
#include "HMUI/ScrollView.hpp"
#include "logging.hpp"
#include "main.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "TMPro/AtlasPopulationMode.hpp"
#include "TMPro/ShaderUtilities.hpp"
#include "TMPro/TMP_Settings.hpp"
#include "TMPro/TMP_Text.hpp"
#include "UnityEngine/AssetBundle.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Shader.hpp"
#include "UnityEngine/TextCore/LowLevel/GlyphRenderMode.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"

#define coro(coroutine) BSML::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(coroutine))

using namespace UnityEngine;

namespace BetterSongSearch::UI::Util::BSMLStuff {
    namespace {
        constexpr auto SYSTEM_FONT_PATH = "/system/fonts/Roboto-Regular.ttf";
        constexpr auto COLOR_EMOJI_FONT_PATH = "/system/fonts/FacebookEmoji64.ttf";
        constexpr auto COLOR_EMOJI_SHADER_ASSET_NAME = "tmp_sprite.shader";
        constexpr auto COLOR_EMOJI_SHADER_BUNDLE_FILE_NAME = "bss-color-emoji-shader.bundle";
        constexpr int SYSTEM_FONT_ATLAS_SIZE = 1024;
        constexpr int SYSTEM_FONT_PADDING = 9;
        constexpr int COLOR_EMOJI_FONT_ATLAS_SIZE = 2048;
        constexpr int COLOR_EMOJI_FONT_POINT_SIZE = 55;
        constexpr float COLOR_EMOJI_SCALE = 0.8f;
        UnityW<TMPro::TMP_FontAsset> systemFont;
        UnityW<TMPro::TMP_FontAsset> colorEmojiFont;
        UnityW<UnityEngine::Material> curvedSystemFontMaterial;
        UnityW<UnityEngine::Material> curvedColorEmojiMaterial;
        UnityW<UnityEngine::AssetBundle> colorEmojiShaderBundle;
        UnityW<UnityEngine::Shader> colorEmojiSpriteShader;
        bool systemFontLoadAttempted = false;
        bool colorEmojiFontLoadAttempted = false;
        bool overrideColorEmojiShaderFind = false;

        struct ScopedColorEmojiShaderFindOverride {
            ScopedColorEmojiShaderFindOverride() {
                overrideColorEmojiShaderFind = true;
            }

            ~ScopedColorEmojiShaderFindOverride() {
                overrideColorEmojiShaderFind = false;
            }
        };

        struct ScopedFallbackMaterialOverride {
            UnityW<TMPro::TMP_Settings> settings = TMPro::TMP_Settings::get_instance();
            bool previous = settings && settings->__cordl_internal_get_m_matchMaterialPreset();

            ScopedFallbackMaterialOverride() {
                if (settings) {
                    settings->__cordl_internal_set_m_matchMaterialPreset(false);
                }
            }

            ~ScopedFallbackMaterialOverride() {
                if (settings) {
                    settings->__cordl_internal_set_m_matchMaterialPreset(previous);
                }
            }
        };

        std::filesystem::path GetColorEmojiShaderBundlePath() {
            return std::filesystem::path(fmt::format(
                "/sdcard/ModData/{}/Mods/{}/Cache/{}",
                modloader::get_application_id().c_str(),
                MOD_ID,
                COLOR_EMOJI_SHADER_BUNDLE_FILE_NAME
            ));
        }

        bool LoadColorEmojiSpriteShader() {
            colorEmojiSpriteShader = UnityEngine::Shader::Find("TextMeshPro/Sprite");
            if (colorEmojiSpriteShader) {
                return true;
            }

            if (!PrepareColorEmojiShaderBundle()) {
                return false;
            }

            colorEmojiShaderBundle = UnityEngine::AssetBundle::LoadFromFile(StringW(GetColorEmojiShaderBundlePath().string()));
            if (!colorEmojiShaderBundle) {
                ERROR("Failed to load color emoji shader bundle");
                return false;
            }

            colorEmojiSpriteShader = colorEmojiShaderBundle->LoadAsset<UnityEngine::Shader*>(COLOR_EMOJI_SHADER_ASSET_NAME);
            if (!colorEmojiSpriteShader) {
                ERROR("Failed to load {} from color emoji shader bundle", COLOR_EMOJI_SHADER_ASSET_NAME);
                return false;
            }

            INFO("Loaded {} shader from embedded Android bundle", (std::string) colorEmojiSpriteShader->get_name());
            return true;
        }

        UnityW<TMPro::TMP_FontAsset> GetColorEmojiFont() {
            if (colorEmojiFont || colorEmojiFontLoadAttempted) {
                return colorEmojiFont;
            }

            colorEmojiFontLoadAttempted = true;
            if (!LoadColorEmojiSpriteShader()) {
                return nullptr;
            }

            try {
                ScopedColorEmojiShaderFindOverride shaderFindOverride;
                colorEmojiFont = TMPro::TMP_FontAsset::CreateFontAsset(
                    StringW(COLOR_EMOJI_FONT_PATH),
                    0,
                    COLOR_EMOJI_FONT_POINT_SIZE,
                    0,
                    UnityEngine::TextCore::LowLevel::GlyphRenderMode::COLOR,
                    COLOR_EMOJI_FONT_ATLAS_SIZE,
                    COLOR_EMOJI_FONT_ATLAS_SIZE,
                    TMPro::AtlasPopulationMode::Dynamic,
                    true
                );

                if (!colorEmojiFont) {
                    ERROR("TMP returned null while loading color emoji font {}", COLOR_EMOJI_FONT_PATH);
                    return nullptr;
                }

                auto faceInfo = colorEmojiFont->get_faceInfo();
                // Fixed bitmap-strike fonts can report zero here, but TMP divides by pointSize when laying out fallback glyphs.
                if (faceInfo.get_pointSize() <= 0) {
                    faceInfo.m_PointSize = COLOR_EMOJI_FONT_POINT_SIZE;
                }
                faceInfo.set_scale(COLOR_EMOJI_SCALE);
                colorEmojiFont->set_faceInfo(faceInfo);

                auto testEmojiAdded = colorEmojiFont->TryAddCharacters(
                    ArrayW<uint32_t>({0x1F600, 0x1F308, 0x1F496, 0x274C}),
                    false
                );
                if (!testEmojiAdded) {
                    WARNING("Could not prewarm all color emoji test glyphs");
                } else {
                    INFO("Prewarmed color emoji glyphs at their native {} px bitmap strike", COLOR_EMOJI_FONT_POINT_SIZE);
                }

                auto uiNoGlow = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>().front_or_default([](auto material) {
                    return material && material->get_name() == std::string_view("UINoGlow");
                });
                if (uiNoGlow) {
                    curvedColorEmojiMaterial = UnityEngine::Material::New_ctor(uiNoGlow);
                    curvedColorEmojiMaterial->set_name("BetterSongSearch Color Emoji Curved");
                    curvedColorEmojiMaterial->set_mainTexture(colorEmojiFont->get_atlasTexture());
                    colorEmojiFont->set_material(curvedColorEmojiMaterial);
                    UnityEngine::Object::DontDestroyOnLoad(curvedColorEmojiMaterial);
                    INFO("Using UINoGlow material for curved color emoji rendering");
                } else {
                    WARNING("UINoGlow material was not found; color emoji will use TextMeshPro/Sprite");
                }

                UnityEngine::Object::DontDestroyOnLoad(colorEmojiFont);
                INFO("Loaded dynamic color emoji TMP font asset from {}", COLOR_EMOJI_FONT_PATH);
            } catch (std::exception const& exception) {
                colorEmojiFont = nullptr;
                ERROR("Failed to load color emoji TTF {}: {}", COLOR_EMOJI_FONT_PATH, exception.what());
            } catch (...) {
                colorEmojiFont = nullptr;
                ERROR("Failed to load color emoji TTF {}", COLOR_EMOJI_FONT_PATH);
            }

            return colorEmojiFont;
        }

        void AddFontFallback(UnityW<TMPro::TMP_FontAsset> font, UnityW<TMPro::TMP_FontAsset> fallback) {
            auto fallbacks = font->get_fallbackFontAssetTable();
            if (!fallbacks) {
                fallbacks = ListW<UnityW<TMPro::TMP_FontAsset>>::New();
                font->set_fallbackFontAssetTable(fallbacks);
            }

            if (!fallbacks->Contains(fallback)) {
                fallbacks->Add(fallback);
                font->ClearFallbackCharacterTable();
            }
        }
    }

    MAKE_HOOK_MATCH(
        Shader_Find_ColorEmoji,
        &UnityEngine::Shader::Find,
        UnityW<UnityEngine::Shader>,
        StringW name
    ) {
        if (overrideColorEmojiShaderFind && colorEmojiSpriteShader && name == std::string_view("TextMeshPro/Sprite")) {
            return colorEmojiSpriteShader;
        }
        return Shader_Find_ColorEmoji(name);
    }

    void InstallColorEmojiShaderHook() {
        INSTALL_HOOK(Logger, Shader_Find_ColorEmoji);
    }

    bool PrepareColorEmojiShaderBundle() {
        auto path = GetColorEmojiShaderBundlePath();
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            ERROR("Failed to create color emoji shader cache directory {}: {}", path.parent_path().string(), error.message());
            return false;
        }

        auto bundle = static_cast<std::span<uint8_t>>(Assets::bss_color_emoji_shader_bundle);
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            ERROR("Failed to open color emoji shader cache file {}", path.string());
            return false;
        }

        output.write(reinterpret_cast<char const*>(bundle.data()), bundle.size());
        output.close();
        if (!output) {
            ERROR("Failed to write color emoji shader cache file {}", path.string());
            return false;
        }

        DEBUG("Prepared color emoji shader bundle at {}", path.string());
        return true;
    }

    UnityW<TMPro::TMP_FontAsset> GetSystemFont() {
        if (systemFont || systemFontLoadAttempted) {
            return systemFont;
        }

        systemFontLoadAttempted = true;
        auto mainUiMaterial = BSML::Helpers::GetMainUIFontMaterial();
        auto previousMobileSdfShader = TMPro::ShaderUtilities::getStaticF_k_ShaderRef_MobileSDF();
        bool restoreMobileSdfShader = false;

        if (mainUiMaterial) {
            auto curvedShader = mainUiMaterial->get_shader();
            if (curvedShader) {
                TMPro::ShaderUtilities::setStaticF_k_ShaderRef_MobileSDF(curvedShader);
                restoreMobileSdfShader = true;
            }
        }

        try {
            systemFont = TMPro::TMP_FontAsset::CreateFontAsset(
                StringW(SYSTEM_FONT_PATH),
                0,
                90,
                SYSTEM_FONT_PADDING,
                UnityEngine::TextCore::LowLevel::GlyphRenderMode::SDFAA,
                SYSTEM_FONT_ATLAS_SIZE,
                SYSTEM_FONT_ATLAS_SIZE,
                TMPro::AtlasPopulationMode::Dynamic,
                true
            );

            if (systemFont) {
                auto emojiFont = GetColorEmojiFont();
                if (emojiFont) {
                    AddFontFallback(systemFont, emojiFont);
                    INFO("Added color emoji font as a Roboto fallback");
                }

                if (mainUiMaterial) {
                    curvedSystemFontMaterial = UnityEngine::Material::New_ctor(mainUiMaterial);
                    curvedSystemFontMaterial->set_name("BetterSongSearch Roboto Curved");
                    curvedSystemFontMaterial->SetTexture(TMPro::ShaderUtilities::getStaticF_ID_MainTex(), systemFont->get_atlasTexture());
                    curvedSystemFontMaterial->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_TextureWidth(), SYSTEM_FONT_ATLAS_SIZE);
                    curvedSystemFontMaterial->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_TextureHeight(), SYSTEM_FONT_ATLAS_SIZE);
                    curvedSystemFontMaterial->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_GradientScale(), SYSTEM_FONT_PADDING + 1);
                    curvedSystemFontMaterial->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_WeightNormal(), systemFont->normalStyle);
                    curvedSystemFontMaterial->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_WeightBold(), systemFont->boldStyle);
                    UnityEngine::Object::DontDestroyOnLoad(curvedSystemFontMaterial);
                }

                UnityEngine::Object::DontDestroyOnLoad(systemFont);
                INFO("Loaded curved TMP font asset from {}", SYSTEM_FONT_PATH);
            } else {
                ERROR("TMP returned null while loading {}", SYSTEM_FONT_PATH);
            }
        } catch (std::exception const& exception) {
            ERROR("Failed to load system TTF {}: {}", SYSTEM_FONT_PATH, exception.what());
        } catch (...) {
            ERROR("Failed to load system TTF {}", SYSTEM_FONT_PATH);
        }

        if (restoreMobileSdfShader) {
            TMPro::ShaderUtilities::setStaticF_k_ShaderRef_MobileSDF(previousMobileSdfShader);
        }

        return systemFont;
    }

    void ApplySystemFont(UnityEngine::Transform* container) {
        if (!container) {
            return;
        }

        auto font = GetSystemFont();
        if (!font) {
            return;
        }

        for (auto text : container->GetComponentsInChildren<TMPro::TMP_Text*>(true)) {
            if (text) {
                text->set_font(font);
                auto curvedText = text->get_gameObject()->GetComponent<HMUI::CurvedTextMeshPro*>();
                if (curvedText && curvedSystemFontMaterial) {
                    text->set_fontSharedMaterial(curvedSystemFontMaterial);
                } else {
                    text->set_fontSharedMaterial(font->get_material());
                }
            }
        }
    }

    void ApplyColorEmojiFallback(UnityEngine::Transform* container) {
        if (!container) {
            return;
        }

        auto emojiFont = GetColorEmojiFont();
        if (!emojiFont) {
            return;
        }

        // Emoji uses a bitmap UI shader, so inheriting the primary font's SDF preset would make its atlas transparent.
        ScopedFallbackMaterialOverride fallbackMaterialOverride;

        for (auto text : container->GetComponentsInChildren<TMPro::TMP_Text*>(true)) {
            if (!text) {
                continue;
            }

            auto font = text->get_font();
            if (!font) {
                continue;
            }

            AddFontFallback(font, emojiFont);

            text->SetAllDirty();
            text->ForceMeshUpdate(true, true);
        }
    }

    custom_types::Helpers::Coroutine MergeSliders(GameObject* container, bool constrictValuesMinMax) {
        co_yield nullptr;
        auto items = container->GetComponentsInChildren<HMUI::CurvedTextMeshPro*>();

        // Construct a list of MERGE_TO_PREV
        std::vector<HMUI::CurvedTextMeshPro*> filteredItems = std::vector<HMUI::CurvedTextMeshPro*>();

        for (auto item : items) {
            if (item != nullptr && item->___m_CachedPtr.m_value != nullptr && item->get_text() == "MERGE_TO_PREV") {
                filteredItems.push_back(item);
            }
        }

        for (auto x : filteredItems) {
            co_yield reinterpret_cast<System::Collections::IEnumerator*>(WaitForEndOfFrame::New_ctor());
            auto ourContainer = x->get_transform()->get_parent();
            auto prevContainer = ourContainer->get_parent()->GetChild(ourContainer->GetSiblingIndex() - 1);

            prevContainer->Find("BSMLSlider").cast<UnityEngine::RectTransform>()->set_offsetMax(Vector2(-20, 0));
            ourContainer->Find("BSMLSlider").cast<UnityEngine::RectTransform>()->set_offsetMin(Vector2(-20, 0));
            ourContainer->set_position(prevContainer->get_position());

            auto minTimeSlider = prevContainer->GetComponentInChildren<HMUI::CustomFormatRangeValuesSlider*>();
            auto maxTimeSlider = ourContainer->GetComponentInChildren<HMUI::CustomFormatRangeValuesSlider*>();

            if (minTimeSlider == nullptr || maxTimeSlider == nullptr) {
                co_yield nullptr;
            } else {
                auto newSize = minTimeSlider->get_valueSize() / 2.1f;
                maxTimeSlider->set_valueSize(newSize);
                minTimeSlider->set_valueSize(newSize);
                ourContainer->GetComponentInChildren<UnityEngine::UI::LayoutElement*>()->set_ignoreLayout(true);
                x->set_text("");
            }
        }
    }

    void SetStringSettingValue(BSML::StringSetting* element, std::string value) {
        // auto formatter = element->formatter;
        // auto formattedValue = formatter(formatter ? formatter(value) : StringW(value));
        // element->modalKeyboard->SetText(value);
        element->set_text(value);
        element->ApplyValue();
    }

    void SetSliderSettingValue(BSML::SliderSetting* element, float value) {
        // auto formatter = element->formatter;
        // auto formattedValue = formatter(formatter ? formatter(value) : value);
        // element->slider->set_value(value);
        element->set_Value(value);
        element->ApplyValue();
    }

    void FormatSliderSettingValue(BSML::SliderSetting* element) {
        // Not pretty but works
        auto value = element->get_Value();
        auto formatter = element->formatter;
        element->text->set_text(formatter ? formatter(value) : StringW(fmt::format("{}", value)));
    }

    void FormatStringSettingValue(BSML::StringSetting* element) {
        // Not pretty but works
        auto value = element->get_text();
        auto formatter = element->formatter;
        element->text->set_text(formatter ? formatter(value) : value);
    }
};  // namespace BetterSongSearch::UI::Util::BSMLStuff
