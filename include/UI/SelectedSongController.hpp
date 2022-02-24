#pragma once

#include <utility>

#include "UnityEngine/MonoBehaviour.hpp"

#include "custom-types/shared/macros.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Sprite.hpp"
#include "HMUI/ImageView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"

#include "questui_components/shared/components/Text.hpp"
#include "questui_components/shared/components/Button.hpp"
#include "questui_components/shared/components/Image.hpp"
#include "questui_components/shared/reference_comp.hpp"

#include "CustomComponents.hpp"
#include "shared/components/list/CustomCelledList.hpp"

namespace BetterSongSearch::UI {

    struct SelectedSongController {
        LazyInitAndUpdate<QUC::Button> playButton{"Play",[this](QUC::Button& button, UnityEngine::Transform* transform, QUC::RenderContext& ctx){
            PlaySong();
        },"PlayButton"};

        LazyInitAndUpdate<QUC::Button> downloadButton{"Download",[this](QUC::Button& button, UnityEngine::Transform* transform, QUC::RenderContext& ctx){
            DownloadSong();
        },"PlayButton"};

        LazyInitAndUpdate<QUC::Button> infoButton{"Song Details", nullptr, "PlayButton"};

        LazyInitAndUpdate<QUC::Text> authorText{"Author", true, UnityEngine::Color{0.8f, 0.8f, 0.8f, 1}, 3.2f};
        LazyInitAndUpdate<QUC::Text> songNameText{"Name", true, std::nullopt, 2.7f};
        LazyInitAndUpdate<QUC::Text> infoText{"details"};
        LazyInitAndUpdate<QUC::Image> coverImage{nullptr, UnityEngine::Vector2{28, 28}};
        UnityEngine::Sprite *defaultImage = nullptr;
        QUC::RenderHeldData<SDC_wrapper::BeatStarSong const*> currentSong = nullptr;
        QUC::RenderContext* ctx; // this is ugly, oh well

        const QUC::Key key;

        SelectedSongController() = default;
        SelectedSongController(SDC_wrapper::BeatStarSong const *currentSong, UnityEngine::Sprite *defaultImage)
                : currentSong(currentSong), coverImage(defaultImage, UnityEngine::Vector2(28, 28)),
                  defaultImage(defaultImage) {
            coverImage.child.preserveAspectRatio = true;
        }

        void SetSong(const SDC_wrapper::BeatStarSong *);

        void DownloadSong();

        void PlaySong();

        [[nodiscard]] constexpr auto DefaultAuthorText() {
            QUC::ModifyContentSizeFitter authorFitter(QUC::detail::refComp(authorText));
            authorFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            authorFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            return authorFitter;
        }

        [[nodiscard]] constexpr auto DefaultNameText() {
            QUC::ModifyContentSizeFitter nameFitter(QUC::detail::refComp(songNameText.child));
            nameFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            nameFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            return nameFitter;
        }

        [[nodiscard]] auto MetaText() {
            // Meta
            QUC::detail::VerticalLayoutGroup metaLayout(
                    DefaultAuthorText(),
                    DefaultNameText()
            );

            QUC::ModifyContentSizeFitter contentSizeFitter(metaLayout);
            contentSizeFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            contentSizeFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;


            QUC::detail::VerticalLayoutGroup modifyLayout(
                    contentSizeFitter
            );

            modifyLayout.childForceExpandHeight = true;
            modifyLayout.padding = {2, 2, 2, 2};

            QUC::ModifyLayoutElement layoutElement(modifyLayout);
            layoutElement.preferredWidth = 40;

            return layoutElement;
        }

        [[nodiscard]] constexpr auto DefaultCoverImage() {
            QUC::ModifyLayoutElement coverElement(QUC::detail::refComp(coverImage));
            coverElement.preferredHeight = 28;
            coverElement.preferredWidth = 28;

            QUC::detail::HorizontalLayoutGroup metaLayout(coverElement);

            QUC::ModifyContentSizeFitter metaFitter(metaLayout);
            metaFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            metaFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            QUC::ModifyLayoutElement metaElement(metaFitter);

            metaElement.preferredWidth = 28;
            metaElement.preferredHeight = 28;

            return metaElement;
        }

        [[nodiscard]] constexpr auto DefaultMinMaxDiffInfo() {
            QUC::detail::HorizontalLayoutGroup layout(QUC::detail::refComp(infoText));

            QUC::ModifyContentSizeFitter nameFitter(layout);
            nameFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            nameFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            return nameFitter;
        }

        [[nodiscard]] auto DefaultButtonLayout() {
            QUC::detail::VerticalLayoutGroup layout(
                    QUC::detail::refComp(downloadButton),
                    QUC::detail::refComp(playButton),
                    QUC::detail::refComp(infoButton)
            );

            QUC::ModifyContentSizeFitter nameFitter(layout);
            nameFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            nameFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            return nameFitter;
        }

        void update();

        auto render(QUC::RenderContext &ctx, QUC::RenderContextChildData &data) {
            this->ctx = &ctx;
            auto& rendered = data.getData<bool>();

            if (!rendered) {
                // render layout for buttons
                QUC::detail::VerticalLayoutGroup buttons(
                        MetaText(),
                        DefaultCoverImage(),
                        DefaultMinMaxDiffInfo(),
                        DefaultButtonLayout()
                );


                buttons.childForceExpandHeight = false;
                buttons.padding = {2,2,2,2};

                QUC::ModifyLayoutElement modifyLayoutElement(buttons);
                modifyLayoutElement.preferredWidth = 40;

                QUC::Backgroundable bgButtons("round-rect-panel", true, modifyLayoutElement);

                auto transform = QUC::detail::renderSingle(bgButtons, ctx);

                return &data.getChildContext([transform]{return transform; }).parentTransform;

            } else {
                // Just update the existing components
                update();
            }

            rendered = true;

            return &data.getChildContext([]{
                SAFE_ABORT(); // not possible
                return nullptr;
            }).parentTransform;
        }
    };
}

inline GlobalNamespace::IPreviewBeatmapLevel* currentLevel;
inline bool inBSS;