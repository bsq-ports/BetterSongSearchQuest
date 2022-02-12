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

    struct SelectedSongController : public QUC::detail::VerticalLayoutGroup<> {
        void SetSong(const SDC_wrapper::BeatStarSong *);

        void DownloadSong();

        void PlaySong();


        [[nodiscard]] auto DefaultAuthorText() {
            ModifyContentSizeFitter authorFitter(QUC::detail::refComp(authorText));
            authorFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            authorFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            return authorFitter;
        }

        [[nodiscard]] auto DefaultNameText() {
            ModifyContentSizeFitter nameFitter(QUC::detail::refComp(songNameText.child));
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

            ModifyContentSizeFitter contentSizeFitter(metaLayout);
            contentSizeFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            contentSizeFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            ModifyLayout modifyLayout = ModifyLayout(
                    QUC::detail::VerticalLayoutGroup(
                            metaLayout
                    )
            );

            modifyLayout.childForceExpandHeight = true;
            modifyLayout.padding = {2, 2, 2, 2};

            ModifyLayoutElement layoutElement(modifyLayout);
            layoutElement.preferredWidth = 40;

            return layoutElement;
        }

        [[nodiscard]] auto DefaultCoverImage() {
            ModifyLayoutElement coverElement(QUC::detail::refComp(coverImage));
            coverElement.preferredHeight = 28;
            coverElement.preferredWidth = 28;


            QUC::detail::HorizontalLayoutGroup metaLayout(coverElement);

            ModifyContentSizeFitter metaFitter(metaLayout);
            metaFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            metaFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            ModifyLayoutElement metaElement(metaFitter);

            metaElement.preferredWidth = 28;
            metaElement.preferredHeight = 28;


            return metaElement;
        }

        [[nodiscard]] auto DefaultMinMaxDiffInfo() {
            QUC::detail::HorizontalLayoutGroup layout(QUC::detail::refComp(infoText));

            ModifyContentSizeFitter nameFitter(layout);
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

            ModifyContentSizeFitter nameFitter(layout);
            nameFitter.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            nameFitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            return nameFitter;
        }

        const SDC_wrapper::BeatStarSong *currentSong;

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

        QUC::RenderContext *ctx = nullptr; // this is ugly but whatever

        SelectedSongController() = default;
        SelectedSongController(SDC_wrapper::BeatStarSong const *currentSong, UnityEngine::Sprite *defaultImage)
                : currentSong(currentSong), coverImage(defaultImage, UnityEngine::Vector2(28, 28)),
                  defaultImage(defaultImage) {
            coverImage.child.preserveAspectRatio = true;
        }

        auto render(QUC::RenderContext &ctx, QUC::RenderContextChildData &data) {
            this->ctx = &ctx;

            auto &viewLayout = data.getData<UnityEngine::UI::VerticalLayoutGroup *>();

            if (!viewLayout) {
                QUC::detail::VerticalLayoutGroup<>::render(ctx, data);
                QUC::RenderContext &childrenCtx = data.getChildContext([viewLayout] {
                    return viewLayout->get_transform();
                });

                QUC::detail::Container renderlayout(
                        MetaText(),
                        DefaultCoverImage(),
                        DefaultMinMaxDiffInfo(),
                        DefaultButtonLayout()
                );


                QUC::detail::renderSingle(renderlayout, childrenCtx);

            } else {
                // Just update the existing components
                QUC::RenderContext &childrenCtx = data.getChildContext([viewLayout] {
                    return viewLayout->get_transform();
                });

                playButton.update();
                downloadButton.update();
                authorText.update();
                songNameText.update();
                infoText.update();
                coverImage.update();
            }
        }

    };
}

inline GlobalNamespace::IPreviewBeatmapLevel* currentLevel;
inline bool inBSS;