#pragma once

#include "UnityEngine/MonoBehaviour.hpp"
#include "HMUI/TableView_IDataSource.hpp"

#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/TableCell.hpp"
#include "System/Object.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextAlignmentOptions.hpp"

#include "songloader/shared/API.hpp"

#include "questui/shared/CustomTypes/Components/List/QuestUITableView.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/SegmentedControl/CustomTextSegmentedControlData.hpp"

#include "sdc-wrapper/shared/BeatStarSong.hpp"
#include "sdc-wrapper/shared/BeatStarCharacteristic.hpp"
#include "sdc-wrapper/shared/BeatStarSongDifficultyStats.hpp"

#include "UI/SelectedSongController.hpp"
#include "CustomComponents.hpp"

#include "FilterOptions.hpp"

#include "questui_components/shared/state.hpp"
#include "questui_components/shared/context.hpp"
#include "questui_components/shared/components/Text.hpp"
#include "questui_components/shared/components/list/CustomCelledList.hpp"
#include "questui_components/shared/components/misc/Utility.hpp"
#include "questui_components/shared/components/misc/HMUITouchable.hpp"
#include "questui_components/shared/components/LoadingIndicator.hpp"

#include <fmt/chrono.h>

namespace BetterSongSearch::UI {
    struct DataHolder {
        inline static std::unordered_map<std::string, std::unordered_map<std::string, float>> songsWithScores;
        inline static std::unordered_set<const SDC_wrapper::BeatStarSong*> songList;
        inline static std::vector<const SDC_wrapper::BeatStarSong*> filteredSongList;
        inline static bool loadedSDC = false;
        inline static FilterOptions filterOptions;
    };

#define PROP_GET(jsonName, varName)                                \
    static auto jsonNameHash_##varName = stringViewHash(jsonName); \
    if (nameHash == (jsonNameHash_##varName))                      \
        return &varName;


    constexpr std::string_view ShortMapDiffNames(std::string_view input) {
        // order variables from most likely to least likely to at least improve branch checks
        // optimization switch idea stolen from Red's SDC wrapper. Good job red 👏
        switch (input.front()) {
            case 'E':
                // ExpertPlus
                if (input.back() == 's') {
                    return "Ex+";
                }
                // Easy
                if (input.size() == 4) {
                    return "E";
                }
                // Expert
                return "Ex";
            case 'N':
                return "N";
            case 'H':
                return "H";
            default:
                return "UNKNOWN";
        }
    }

#undef PROP_GET

    struct CellData : public QUC::CustomTypeList::QUCDescriptor {
        const SDC_wrapper::BeatStarSong* song;
        constexpr CellData(SDC_wrapper::BeatStarSong const *song) : QUCDescriptor(), song(song) {
            interactable = true;
        }
    };

    struct CellDiffSegmentedControl {
        const QUC::Key key;
        QUC::HeldData<std::vector<SDC_wrapper::BeatStarSongDifficultyStats const*>> diffs;


        auto render(QUC::RenderContext &ctx, QUC::RenderContextChildData &data) {
            auto& segmentedControl = data.getData<QuestUI::CustomTextSegmentedControlData*>();

            bool inited = static_cast<bool>(segmentedControl);

            if (!segmentedControl) {
                getLogger().debug("Building segmented control");
                segmentedControl = QuestUI::BeatSaberUI::CreateTextSegmentedControl(&ctx.parentTransform, nullptr);

                segmentedControl->fontSize = 2;
                segmentedControl->padding = 1.5;
                segmentedControl->overrideCellSize = true;
                constexpr auto handleCellPrefab = [](HMUI::SegmentedControlCell*& cell) {
                    cell = UnityEngine::Object::Instantiate(cell);
                    UnityEngine::Object::Destroy(cell->GetComponentInChildren<HMUI::Touchable*>());
                    auto text = cell->GetComponentInChildren<TMPro::TextMeshProUGUI*>();

                    text->set_fontStyle(TMPro::FontStyles::Normal);
                    text->set_richText(true);

                };

                handleCellPrefab(segmentedControl->firstCellPrefab);
                handleCellPrefab(segmentedControl->lastCellPrefab);
                handleCellPrefab(segmentedControl->middleCellPrefab);
                handleCellPrefab(segmentedControl->singleCellPrefab);

                segmentedControl->hideCellBackground = true;
            }

            if (diffs.isModified() || !inited) {
                std::vector<SDC_wrapper::BeatStarSongDifficultyStats const*> const& diffsVector = diffs.getData();
                // avoid unnecessary allocation
                ArrayW<StringW> strs = segmentedControl->texts.size() == diffsVector.size() ? segmentedControl->texts : ArrayW<StringW>(diffsVector.size());
                int i = 0;
                for (auto const &diffData: diffsVector) {
                    if (diffData->ranked) {
                        strs[i] = fmt::format(FMT_STRING("<color=white>{}</color> <color=#ffa500>{:.1f}</color>"), diffData->GetName(), diffData->stars);
                    } else {
                        if (diffData->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Lightshow())
                            strs[i] = fmt::format(FMT_STRING("<color=#AAAAAA>{}(LS)</color>"), diffData->GetName());
                        else if (diffData->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Lawless())
                            strs[i] = fmt::format(FMT_STRING("<color=#AAAAAA>{}(LL)</color>"), diffData->GetName());
                        else if (diffData->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Degree90())
                            strs[i] = fmt::format(FMT_STRING("<color=#AAAAAA>{}(90)</color>"), diffData->GetName());
                        else if (diffData->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Degree360())
                            strs[i] = fmt::format(FMT_STRING("<color=#AAAAAA>{}(360)</color>"), diffData->GetName());
                        else if (diffData->diff_characteristics == SDC_wrapper::BeatStarCharacteristic::Standard())
                            strs[i] = fmt::format(FMT_STRING("<color=white>{}</color>"), diffData->GetName());
                        else
                            strs[i] = fmt::format(FMT_STRING("<color=#AAAAAA>{}</color>"), ShortMapDiffNames(diffData->GetName()));
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
        QUC::Text mapperText{"Deez Nuts loolollolool", true, Sombrero::FastColor(0.8f, 0.8f, 0.8f, 1), 2.3f};
        QUC::Text songText{"Deez Nuts loolollolool", true, std::nullopt, 2.7f};
        QUC::Text uploadDateText{"Failed To Parse", true, Sombrero::FastColor(0.66f, 0.66f, 0.66f, 1), 2.7f}; // make a comp to format date?
        QUC::Text ratingText{"Deez Nuts loolollolool", true, Sombrero::FastColor(0.8f, 0.8f, 0.8f, 1), 2.7f};
        CellDiffSegmentedControl cellDiff;
        std::function<void(Sombrero::FastColor const&)> setBgColor;

        std::remove_reference_t<T> view;

        const QUC::Key key;

        BasicCellComponent(T&& view) : view(view) {}

        auto cellElementsLayout() {
            getLogger().debug("Building cell layout");


            // top layout

            songText.alignmentOptions = TMPro::TextAlignmentOptions::MidlineLeft;
            songText.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            songText.wordWrapping = false;

            uploadDateText.alignmentOptions = TMPro::TextAlignmentOptions::MidlineRight;
            uploadDateText.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            uploadDateText.wordWrapping = false;

            QUC::detail::HorizontalLayoutGroup topLayout(
                    QUC::detail::refComp(songText),
                    QUC::detail::refComp(uploadDateText)
            );

            topLayout.childControlWidth = true;

            //

            // MID
            mapperText.alignmentOptions = TMPro::TextAlignmentOptions::MidlineLeft;
            mapperText.overflowMode = TMPro::TextOverflowModes::Ellipsis;
            mapperText.wordWrapping = false;

            ratingText.alignmentOptions = TMPro::TextAlignmentOptions::MidlineRight;
            ratingText.wordWrapping = false;


            QUC::detail::HorizontalLayoutGroup midLayout(
                    QUC::detail::refComp(mapperText),
                    QUC::detail::refComp(ratingText)
            );
            //

            // BOTTOM
            QUC::ModifyContentSizeFitter bottomLayout(QUC::HorizontalLayoutGroup(
                    QUC::RefComp(cellDiff)
            ));
            bottomLayout.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
            bottomLayout.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

            QUC::ModifyLayoutElement bottomLayoutElement(bottomLayout);
            bottomLayoutElement.preferredHeight = 3.9f;
            bottomLayoutElement.preferredWidth = 68.0f;
            //


            // full layout

            QUC::detail::VerticalLayoutGroup layout(
                    topLayout,
                    midLayout,
                    QUC::Backgroundable(
                            "round-rect-panel",
                            true,
                            bottomLayoutElement,
                            1.0f,
                            Sombrero::FastColor::black()
                    )
            );
            layout.padding = std::array<float, 4>{1,1,1,2};

            QUC::ModifyContentSizeFitter fitter(layout);
            fitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained;

            QUC::ModifyLayoutElement layoutElement(fitter);
            layoutElement.preferredHeight = 11.75f;
            layoutElement.preferredWidth = 70;

            QUC::OnRenderCallback bgRenderCallback(
                QUC::Backgroundable("round-rect-panel", true,
                    layoutElement,
                    0.6f
                ),
                [this](auto& self, QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
                    auto& backgroundable = data.getData<QuestUI::Backgroundable*>();
                    setBgColor = [&backgroundable](Sombrero::FastColor const& color) {
                        CRASH_UNLESS(backgroundable);
                        backgroundable->background->set_color(color);
                    };
//                    setBgColor = [self, &ctx](Sombrero::FastColor const& color) mutable {
//                        getLogger().debug("Coloring bg");
//                        self.color = color;
//                        QUC::detail::renderSingle(self.child, ctx);
//                        getLogger().debug("Colored bg");
//                    };
                }
            );

            return bgRenderCallback;
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

                QUC::ModifyLayoutElement layoutElement(view);
                layoutElement.preferredHeight = 11.75f;
                layoutElement.preferredWidth = 70;

                QUC::ModifyContentSizeFitter fitter2(layoutElement);

                fitter2.verticalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;
                fitter2.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize;

                //QUC::HMUITouchable touchable(fitter2);

                getLogger().debug("Building cell first time");
                QUC::detail::renderSingle(fitter2, cellCtx);
            }


            auto ranked = cellData.song->GetMaxStarValue() > 0;
            bool downloaded = RuntimeSongLoader::API::GetLevelByHash(cellData.song->hash.string_data).has_value();

            Sombrero::FastColor songColor = Sombrero::FastColor::white();

            // these double assignments wil be optimized out, don't worry about it!
            if (downloaded) {
                songColor = Sombrero::FastColor(0.53f, 0.53f, 0.53f, 1.0f);
            }

            if (ranked) {
                songColor = UnityEngine::Color(1, 0.647f, 0, 1);
            }

            songText.text = fmt::format("{} - {}", songData->GetName(), songData->GetSongAuthor());
            songText.color = songColor;
            mapperText.text = cellData.song->GetAuthor();
            uploadDateText.text = fmt::format("{:%d. %b %Y}", fmt::localtime(songData->uploaded_unix_time));
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

    QUC::LoadingIndicator loadingIndicator{QUC::LoadingIndicator()};
    std::optional<LazyInitAndUpdate<QUC::detail::VerticalLayoutGroup<QUC::detail::HorizontalLayoutGroup<QUC::RefComp<decltype(loadingIndicator)>>>>> loadingIndicatorContainer;

    using TableType = ConditionalRender<QUC::RecycledTable<BetterSongSearch::UI::QUCObjectTableData, CellComponent>>;

    TableType table {std::initializer_list<BetterSongSearch::UI::CellData>() ,QUC::CustomTypeList::QUCTableInitData("BSS_ReusableUI", 11.7f, UnityEngine::Vector2(70, 6 * 11.7f))};

    BetterSongSearch::UI::QUCObjectTableData* tablePtr;

    QUC::RenderContext ctx{nullptr}; // initialized at DidActivate
)

void Sort(bool resetTable);
extern BetterSongSearch::UI::ViewControllers::SongListViewController* songListController;