#pragma once

#include "UnityEngine/MonoBehaviour.hpp"
#include "HMUI/TableView_IDataSource.hpp"

#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableCell.hpp"
#include "System/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/SegmentedControl/CustomTextSegmentedControlData.hpp"

#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"

#include "UI/SelectedSongController.hpp"
#include "CustomComponents.hpp"

#include "FilterOptions.hpp"

#include "questui_components/shared/context.hpp"
#include "questui_components/shared/components/list/CustomCelledList.hpp"

#include <fmt/chrono.h>

namespace BetterSongSearch::UI {
    struct DataHolder {
        inline static std::unordered_set<const SDC_wrapper::BeatStarSong*> downloadedSongList;
        inline static std::unordered_set<const SDC_wrapper::BeatStarSong*> songList;
        inline static std::vector<const SDC_wrapper::BeatStarSong*> filteredSongList;
        inline static bool loadedSDC = false;
        inline static FilterOptions filterOptions;
    };

    constexpr std::string_view ShortMapDiffNames(std::string_view input) {
        if(input == "Easy")
            return "E";
        if(input == "Normal")
            return "N";
        if(input == "Hard")
            return "H";
        if(input == "Expert")
            return "Ex";
        if(input == "ExpertPlus")
            return "E+";

        return "UNKNOWN";
    }

    struct CellData : public QUC::CustomTypeList::QUCDescriptor {
        const SDC_wrapper::BeatStarSong* song;

        CellData(SDC_wrapper::BeatStarSong const *song) : QUCDescriptor(), song(song) {}
    };

    struct CellDiffSegmentedControl {
        const QUC::Key key;
        QUC::HeldData<std::vector<SDC_wrapper::BeatStarSongDifficultyStats const*>> diffs;
        bool ranked = false;


        auto render(QUC::RenderContext &ctx, QUC::RenderContextChildData &data) {
            auto& segmentedControl = data.getData<QuestUI::CustomTextSegmentedControlData*>();

            if (!segmentedControl) {
                segmentedControl = QuestUI::BeatSaberUI::CreateTextSegmentedControl(&ctx.parentTransform, nullptr);

                segmentedControl->fontSize = 2;
                segmentedControl->padding = 1.5;
                segmentedControl->overrideCellSize = true;
                segmentedControl->firstCellPrefab = UnityEngine::Object::Instantiate(segmentedControl->firstCellPrefab);
                segmentedControl->lastCellPrefab = UnityEngine::Object::Instantiate(segmentedControl->lastCellPrefab);
                segmentedControl->middleCellPrefab = UnityEngine::Object::Instantiate(segmentedControl->middleCellPrefab);
                segmentedControl->singleCellPrefab = UnityEngine::Object::Instantiate(segmentedControl->singleCellPrefab);

                UnityEngine::Object::Destroy(segmentedControl->firstCellPrefab->GetComponentInChildren<HMUI::Touchable*>());
                UnityEngine::Object::Destroy(segmentedControl->lastCellPrefab->GetComponentInChildren<HMUI::Touchable*>());
                UnityEngine::Object::Destroy(segmentedControl->middleCellPrefab->GetComponentInChildren<HMUI::Touchable*>());
                UnityEngine::Object::Destroy(segmentedControl->singleCellPrefab->GetComponentInChildren<HMUI::Touchable*>());

                // TODO: Can we QUC-ify this? I cannot be bothered to figure this out today
                segmentedControl->singleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
                segmentedControl->lastCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
                segmentedControl->firstCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
                segmentedControl->middleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
                segmentedControl->firstCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
                segmentedControl->lastCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
                segmentedControl->middleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
                segmentedControl->singleCellPrefab->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_richText(true);
                segmentedControl->hideCellBackground = true;
            }

            if (diffs.isModified()) {
                std::vector<SDC_wrapper::BeatStarSongDifficultyStats const*> const& diffsVector = diffs.getData();
                ArrayW<StringW> strs(diffsVector.size());
                int i = 0;
                for (auto const &diffData: diffsVector) {
                    if (ranked) {
                        strs[i] = fmt::format("<color=white>{}</color> <color=#ffa500>{:.1f}</color>", diffData->GetName(), diffData->stars);
                    } else {
                        if (diffData->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Lightshow()) {
                            strs[i] = fmt::format("<color=white>{}</color>", ShortMapDiffNames(diffData->GetName()));
                        } else {
                            strs[i] = fmt::format("<color=white>{}</color>", diffData->GetName());
                        }
                    }
                    i++;
                }
                segmentedControl->set_texts(strs);
                diffs.clear();
            }

            return segmentedControl->get_transform();
        }
    };

    // Funny template magic so we can have a field deduced by a method return type
    template<typename T>
    struct BasicCellComponent {
        ModifyText mapperText{QUC::Text("Deez Nuts loolollolool", true, Sombrero::FastColor(0.8f, 0.8f, 0.8f, 1), 2.3f)};
        ModifyText songText{QUC::Text("Deez Nuts loolollolool", true, std::nullopt, 2.7f)};
        ModifyText uploadDateText{QUC::Text("Failed To Parse", true, Sombrero::FastColor(0.66f, 0.66f, 0.66f, 1), 2.7f)}; // make a comp to format date?
        ModifyText ratingText{QUC::Text("Deez Nuts loolollolool", true, Sombrero::FastColor(0.8f, 0.8f, 0.8f, 1), 2.7f)};
        CellDiffSegmentedControl cellDiff;
        std::function<void(Sombrero::FastColor const&)> setBgColor;

        std::remove_reference_t<T> view;

        const QUC::Key key;

        BasicCellComponent(T&& view) : view(view) {}

        auto cellElementsLayout() {
            // top layout
            songText.alignmentOptions = TMPro::TextAlignmentOptions::MidlineLeft;
            songText.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            songText.wordWrapping = false;

            uploadDateText.alignmentOptions = TMPro::TextAlignmentOptions::MidlineLeft;
            uploadDateText.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            uploadDateText.wordWrapping = false;

            ModifyLayout topLayout(QUC::HorizontalLayoutGroup(
                    QUC::RefComp(songText),
                    QUC::RefComp(uploadDateText)
            ));
            topLayout.childForceExpandWidth = true;
            //

            // MID
            mapperText.alignmentOptions = TMPro::TextAlignmentOptions::MidlineLeft;
            mapperText.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            mapperText.wordWrapping = false;


            QUC::detail::HorizontalLayoutGroup midLayout(
                    QUC::detail::refComp(mapperText),
                    QUC::RefComp(ratingText)
            );
            //

            // BOTTOM
            ModifyContentSizeFitter bottomLayout(QUC::HorizontalLayoutGroup(
                    QUC::RefComp(cellDiff)
            ));
            bottomLayout.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            bottomLayout.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            ModifyLayoutElement bottomLayoutElement(bottomLayout);
            bottomLayout.horizontalFit = 3.9f;
            bottomLayout.verticalFit = 68.0f;
            //


            // full layout
            ModifyLayout layout (
                OnRenderCallback(
                    QUC::Backgroundable("round-rect-panel", true,
                         QUC::VerticalLayoutGroup(
                            topLayout,
                            midLayout,
                            QUC::Backgroundable(
                                    "round-rect-panel",
                                    true,
                                    bottomLayoutElement,
                                    1.0f,
                                    Sombrero::FastColor::black()
                            )
                        ),
                    0.6f
                    ),
                    [this](auto& self, QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
                        setBgColor = [self, &ctx](Sombrero::FastColor const& color) mutable {
                            self.color = color;
                            QUC::detail::renderSingle(self.child, ctx);
                        };
                    }
                )
            );

            layout.padding = {1,1, 1, 2};

            ModifyContentSizeFitter fitter(layout);
            fitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained;

            ModifyLayoutElement layoutElement(fitter);
            layoutElement.preferredHeight = 11.75f;
            layoutElement.preferredWidth = 70;

            return layoutElement;
        }

    };

    using NotBasicCellComponent = BasicCellComponent<std::invoke_result_t<decltype(&BasicCellComponent<int>::cellElementsLayout), BasicCellComponent<int>*>>;

    struct CellComponent : public NotBasicCellComponent {
        CellComponent() : NotBasicCellComponent(NotBasicCellComponent::cellElementsLayout()) {}

        void render(CellData const& cellData, QUC::RenderContext& cellCtx) {
            auto& inited = cellCtx.getChildDataOrCreate(key).getData<bool>();

            auto songData = cellData.song;

            if (!inited) {
                // Modify the cell itself
                WrapParent parent(view);
                ModifyContentSizeFitter fitter2(parent);
                fitter2.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
                fitter2.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

                HMUITouchable<decltype(fitter2)> touchable(fitter2);

                QUC::detail::renderSingle(touchable, cellCtx);
            }


            auto ranked = cellData.song->GetMaxStarValue() > 0;
            bool downloaded = DataHolder::downloadedSongList.contains(songData);

            songText.text = fmt::format("{} - {}", songData->GetName(), songData->GetSongAuthor());
            Sombrero::FastColor songColor = Sombrero::FastColor::white();

            // these double assignments wil be optimized out, don't worry about it!
            if (downloaded) {
                songColor = Sombrero::FastColor(0.53f, 0.53f, 0.53f, 1.0f);
            }

            if (ranked) {
                songColor = UnityEngine::Color(1, 0.647f, 0, 1);
            }

            songText.color = songColor;
            mapperText.text = cellData.song->GetAuthor();
            uploadDateText.text = fmt::format("{:%d-%m-%Y}", fmt::localtime(songData->uploaded_unix_time));
            ratingText.text = fmt::format("Length: {:%M:%S} Upvotes: {}, Downvotes: {}", std::chrono::seconds(songData->duration_secs), songData->upvotes, songData->downvotes);
            cellDiff.diffs = songData->GetDifficultyVector();


            QUC::detail::renderSingle(view, cellCtx);
            inited = true;
        }
    };

    static_assert(QUC::ComponentCellRenderable<CellComponent, CellData>);
}

// Header
DECLARE_QUC_TABLE_CELL(BetterSongSearch::UI, QUCObjectTableCell,
           DECLARE_OVERRIDE_METHOD(void, SelectionDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::SelectionDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);
           DECLARE_OVERRIDE_METHOD(void, HighlightDidChange, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::SelectableCell::HighlightDidChange>::get(), HMUI::SelectableCell::TransitionType transitionType);

           CellComponent* cellComp;
           void RefreshVisuals();

public:
           void render(CellData const& cellData, QUC::RenderContext& ctx, BetterSongSearch::UI::CellComponent& cellComp);
)

DECLARE_QUC_TABLE_DATA(BetterSongSearch::UI, QUCObjectTableData, BetterSongSearch::UI::CellData, QUCObjectTableCell, );

static_assert(QUC::HMUICellQUC<BetterSongSearch::UI::QUCObjectTableCell,BetterSongSearch::UI::CellData, BetterSongSearch::UI::CellComponent>);

#define GET_FIND_METHOD(mPtr) il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::get()

// ):
static std::vector<Il2CppClass*> GetInterfaces() {
	return { classof(HMUI::TableView::IDataSource*) };
}



DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::ViewControllers, SongListViewController, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, GET_FIND_METHOD(&HMUI::ViewController::DidActivate), bool firstActivation, bool addedToHeirarchy, bool screenSystemDisabling);

public:
    DECLARE_DEFAULT_CTOR();
    DECLARE_SIMPLE_DTOR();

public:
    TMPro::TextMeshProUGUI* songDetailsText;
    LazyInitAndUpdate<BetterSongSearch::UI::SelectedSongController> selectedSongController;

    using TableType = LazyInitAndUpdate<QUC::RecycledTable<BetterSongSearch::UI::QUCObjectTableData, CellComponent>>;

    TableType table {std::initializer_list<BetterSongSearch::UI::CellData>() ,QUC::CustomTypeList::QUCTableInitData("BSS_ReusableUI", 11.7f, UnityEngine::Vector2(70, 6 * 11.7f))};

    BetterSongSearch::UI::QUCObjectTableData* tablePtr;

    QUC::RenderContext ctx{nullptr}; // initialized at DidActivate
)

void Sort();
extern BetterSongSearch::UI::ViewControllers::SongListViewController* songListController;