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

#include <map>

namespace BetterSongSearch::UI {

    struct SelectedSongController {
        LazyInitAndUpdate<HideObject<QUC::Button>> playButton{"Play",[this](QUC::Button& button, UnityEngine::Transform* transform, QUC::RenderContext& ctx){
            PlaySong();
        },"PlayButton", true, false};

        LazyInitAndUpdate<HideObject<QUC::Button>> downloadButton{"Download",[this](QUC::Button& button, UnityEngine::Transform* transform, QUC::RenderContext& ctx){
            DownloadSong();
        },"PlayButton", true, false};

        QUC::Button infoButton{"Song Details", nullptr, "PlayButton", true, false};

        QUC::Text authorText{"Author", true, UnityEngine::Color{0.8f, 0.8f, 0.8f, 1}, 3.2f};
        QUC::Text songNameText{"Name", true, std::nullopt, 2.7f};
        QUC::Text infoText{"details"};
        LazyInitAndUpdate<QUC::Image> coverImage{nullptr, UnityEngine::Vector2{28, 28}};
        UnityEngine::Sprite *defaultImage = nullptr;
        QUC::RenderHeldData<SDC_wrapper::BeatStarSong const*> currentSong = nullptr;
        QUC::RenderContext* ctx; // this is ugly, oh well

        std::unordered_map<std::string, std::vector<uint8_t>> imageCoverCache = std::unordered_map<std::string, std::vector<uint8_t>>();

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
            QUC::ModifyContentSizeFitter nameFitter(QUC::detail::refComp(songNameText));
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
            modifyLayout.padding = std::array<float, 4>{2, 2, 2, 2};

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

        auto buildView(QUC::RenderContext &ctx, QUC::RenderContextChildData &data) {
            // render layout for buttons
            QUC::detail::VerticalLayoutGroup buttons(
                    MetaText(),
                    DefaultCoverImage(),
                    DefaultMinMaxDiffInfo(),
                    DefaultButtonLayout()
            );


            buttons.childForceExpandHeight = false;
            buttons.padding = std::array<float, 4>{2, 2, 2, 2};

            QUC::ModifyLayoutElement modifyLayoutElement(buttons);
            modifyLayoutElement.preferredWidth = 40;

            QUC::Backgroundable bgButtons("round-rect-panel", true, modifyLayoutElement);

            return bgButtons;
        }

        [[nodiscard]] constexpr auto& getView() const {
            auto& data = ctx->getChildData(key);
            auto& renderedView = data.getData<std::optional<std::result_of_t<decltype(&SelectedSongController::buildView)(SelectedSongController, QUC::RenderContext&, QUC::RenderContextChildData &)>>>();

            return renderedView;
        }

        constexpr auto updateView() const {
            auto& data = ctx->getChildData(key);
            auto& renderedView = getView();
            auto transform = QUC::detail::renderSingle(*renderedView, *ctx);

            return &data.childContext->parentTransform;
        }

        constexpr auto render(QUC::RenderContext &ctx, QUC::RenderContextChildData &data) {
            this->ctx = &ctx;

//            std::result_of_t<decltype(&SelectedSongController::buildView)(SelectedSongController, QUC::RenderContext&, QUC::RenderContextChildData &)> view;
//            std::invoke_result_t<decltype(&SelectedSongController::buildView), SelectedSongController*, QUC::RenderContext &, QUC::RenderContextChildData &> view;

            // this is rather wasteful but whatever
            auto& renderedView = getView();
            if (!renderedView) {
                renderedView.emplace(buildView(ctx, data));
                auto transform = updateView();

                return &data.getChildContext([transform]{return transform; }).parentTransform;
            } else {
                // Just update the existing components
                updateView();
            }

            return &data.getChildContext([]{
                SAFE_ABORT(); // not possible
                return nullptr;
            }).parentTransform;
        }
    };
}

inline GlobalNamespace::IPreviewBeatmapLevel* currentLevel;
inline bool inBSS;