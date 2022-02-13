#pragma once

#include "UnityEngine/Transform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/RectOffset.hpp"

#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/WeakPtrGO.hpp"

#include "questui_components/shared/components/layouts/VerticalLayoutGroup.hpp"
#include "questui_components/shared/components/layouts/HorizontalLayoutGroup.hpp"
#include "questui_components/shared/components/settings/StringSetting.hpp"
#include "questui_components/shared/components/settings/DropdownSetting.hpp"
#include "questui_components/shared/components/Text.hpp"
#include "questui_components/shared/components/Backgroundable.hpp"
#include "HMUI/Touchable.hpp"

#include "main.hpp"

namespace BetterSongSearch::UI {

    // Component to return the transform of the parent
    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    struct WrapParent : public QUC::detail::Container<TArgs...> {
        constexpr WrapParent(TArgs... args) : QUC::detail::Container<TArgs...>(args...) {}
        constexpr WrapParent(std::tuple<TArgs...> args) : QUC::detail::Container<TArgs...>(args) {}

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            QUC::detail::Container<TArgs...>::render(ctx, data);
            return &ctx.parentTransform;
        }
    };

    template<class T>
    requires (QUC::renderable_return<T, UnityEngine::Transform*>)
    struct HMUITouchable {
        std::remove_reference_t<T> child;
        const QUC::Key key;

        HMUITouchable(T child_)
                : child(child_) {}

        UnityEngine::Transform* render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            auto res = QUC::detail::renderSingle(child, ctx);
            auto touchable = data.getData<HMUI::Touchable*>();

            if (!touchable) {
                auto go = res->get_gameObject();
                touchable = go->template AddComponent<HMUI::Touchable*>();
            }

            return res;
        }


    };

    template<class T>
    requires (QUC::renderable_return<T, UnityEngine::Transform*>)
    struct OnRenderCallback {
        std::remove_reference_t<T> child;
        const QUC::Key key;

        using Callback = std::function<void(T& child, QUC::RenderContext& ctx, QUC::RenderContextChildData& data)>;

        Callback renderCallback;

        template<typename F>
        OnRenderCallback(T child_, F&& c)
                : child(child_), renderCallback(c), key(child_.key) {} // use child's key since we just wrap

        UnityEngine::Transform* render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            auto res = QUC::detail::renderSingle(child, ctx);

            if (renderCallback) {
                renderCallback(child, ctx, data);
            }

            return res;
        }


    };

    struct ModifyText : public QUC::Text {
        QUC::HeldData<std::optional<TMPro::TextAlignmentOptions>> alignmentOptions;
        QUC::HeldData<std::optional<TMPro::TextOverflowModes>> overflowMode;
        QUC::HeldData<std::optional<bool>> wordWrapping;

        template<typename... TArgs>
        ModifyText(QUC::Text text, std::optional<TMPro::TextAlignmentOptions> alignment = std::nullopt,
                   std::optional<TMPro::TextOverflowModes> overflowMode = std::nullopt,
                   std::optional<bool> wordWrapping = std::nullopt)
        : QUC::Text(std::move(text)),
        alignmentOptions(alignment), overflowMode(overflowMode), wordWrapping(wordWrapping){}




        void render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            auto& textComp = data.getData<TMPro::TextMeshProUGUI*>();
            auto existed = static_cast<bool>(textComp);
            Text::render(ctx, data);

            if ((!existed || alignmentOptions.isModified()) && *alignmentOptions) {
                textComp->set_alignment(*alignmentOptions.getData());
                alignmentOptions.clear();
            }

            if ((!existed || overflowMode.isModified()) && *overflowMode) {
                textComp->set_overflowMode(*overflowMode.getData());
                overflowMode.clear();
            }

            if ((!existed || wordWrapping.isModified()) && *wordWrapping) {
                textComp->set_enableWordWrapping(*wordWrapping.getData());
                wordWrapping.clear();
            }
        }
    };

    // TODO: Move this to QUC proper?
    template<typename Layout>
    requires (QUC::renderable<Layout>)
    struct ModifyLayout : public Layout {
        constexpr ModifyLayout(Layout layout) : Layout(layout) {}

        QUC::HeldData<UnityEngine::TextAnchor> childAlignment;
        QUC::RenderHeldData<bool> childControlWidth;
        QUC::RenderHeldData<bool> childControlHeight;
        QUC::RenderHeldData<bool> childForceExpandHeight;
        QUC::RenderHeldData<bool> childForceExpandWidth;
        QUC::RenderHeldData<bool> childScaleWidth;
        QUC::RenderHeldData<float> spacing;

        QUC::HeldData<std::array<float, 4>> padding; // TODO: How to make this not stupid


        auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            auto ret = Layout::render(ctx, data);
            auto &viewLayout = data.getData<UnityEngine::UI::HorizontalOrVerticalLayoutGroup *>();

            if (childAlignment.isModified()) {
                viewLayout->set_childAlignment(childAlignment.getData());
                childAlignment.clear();
            }

            if (childControlHeight.readAndClear<false>(ctx)) {
                viewLayout->set_childControlHeight(childControlHeight.getData());
            }

            if (childControlWidth.readAndClear<false>(ctx)) {
                viewLayout->set_childControlWidth(childControlWidth.getData());
            }

            if (childForceExpandHeight.readAndClear<false>(ctx)) {
                viewLayout->set_childForceExpandHeight(childForceExpandHeight.getData());
            }

            if (childForceExpandWidth.readAndClear<false>(ctx)) {
                viewLayout->set_childForceExpandWidth(childForceExpandWidth.getData());
            }

            if (childScaleWidth.readAndClear<false>(ctx)) {
                viewLayout->set_childScaleWidth(childScaleWidth.getData());
            }

            if (spacing.readAndClear<false>(ctx)) {
                viewLayout->set_spacing(spacing.getData());
            }

            if (padding.isModified()) {
                auto const& arr = *padding;
                viewLayout->set_padding(UnityEngine::RectOffset::New_ctor(arr[0], arr[1], arr[2], arr[3]));
                padding.clear();
            }

            return ret;
        }
    };

    // TODO: Move this to QUC proper?
    template<typename Layout>
    requires (QUC::renderable_return<Layout, UnityEngine::Transform*>)
    struct ModifyLayoutElement {
        const QUC::Key key;
        std::remove_reference_t<Layout> layout;

        constexpr ModifyLayoutElement(Layout layout) : layout(layout) {}

        QUC::RenderHeldData<float> preferredWidth;
        QUC::RenderHeldData<float> preferredHeight;

        QUC::RenderHeldData<float> minWidth;
        QUC::RenderHeldData<float> minHeight;

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            UnityEngine::Transform* ret = QUC::detail::renderSingle(layout, ctx);
            auto& layoutElement = data.getData<UnityEngine::UI::LayoutElement*>();

            if (!layoutElement) {
                auto go = ret->get_gameObject();
                layoutElement = go->GetComponent<UnityEngine::UI::LayoutElement*>();
                if (!layoutElement) {
                    layoutElement = go->AddComponent<UnityEngine::UI::LayoutElement*>();
                }
            }

            if (preferredWidth.readAndClear<false>(ctx)) {
                layoutElement->set_preferredWidth(preferredWidth.getData());
            }

            if (preferredHeight.readAndClear<false>(ctx)) {
                layoutElement->set_preferredHeight(preferredHeight.getData());
            }

            if (minHeight.readAndClear<false>(ctx)) {
                layoutElement->set_minHeight(minHeight.getData());
            }

            if (minWidth.readAndClear<false>(ctx)) {
                layoutElement->set_minWidth(minWidth.getData());
            }

            return ret;
        }

    };

    // TODO: Move this to QUC proper?
    template<typename Layout>
    requires (QUC::renderable_return<Layout, UnityEngine::Transform*>)
    struct ModifyContentSizeFitter {
        const QUC::Key key;
        std::remove_reference_t<Layout> layout;

        constexpr ModifyContentSizeFitter(Layout layout) : layout(layout) {}

        QUC::HeldData<UnityEngine::UI::ContentSizeFitter::FitMode> horizontalFit;
        QUC::HeldData<UnityEngine::UI::ContentSizeFitter::FitMode> verticalFit;

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            UnityEngine::Transform* ret = QUC::detail::renderSingle(layout, ctx);
            auto& layoutSizeFitter = data.getData<UnityEngine::UI::ContentSizeFitter*>();

            if (!layoutSizeFitter) {
                auto go = ret->get_gameObject();
                layoutSizeFitter = go->GetComponent<UnityEngine::UI::ContentSizeFitter*>();
                if (!layoutSizeFitter) {
                    layoutSizeFitter = go->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
                }
            }

            if (horizontalFit.isModified()) {
                layoutSizeFitter->set_horizontalFit(*horizontalFit);
                horizontalFit.clear();
            }

            if (verticalFit.isModified()) {
                layoutSizeFitter->set_verticalFit(*verticalFit);
                verticalFit.clear();
            }

            return ret;
        }

    };

    template<typename T>
    struct LazyInitAndUpdate {
        const QUC::Key key;

        LazyInitAndUpdate() = default;

        LazyInitAndUpdate(LazyInitAndUpdate const&) = delete;
        LazyInitAndUpdate(T child) : child(std::move(child)) {}

        template<typename... TArgs>
        LazyInitAndUpdate(TArgs&&... args) : child(std::forward<TArgs>(args)...) {}

        LazyInitAndUpdate& operator =(LazyInitAndUpdate const&) = default;
        LazyInitAndUpdate& operator =(LazyInitAndUpdate&&) noexcept = default;

        QUC::RenderContext* ctx;
        std::remove_reference_t<T> child;

        constexpr LazyInitAndUpdate<T>& emplace(T const& n) {
            child = n;
            return *this;
        }

        template<typename... TArgs>
        constexpr LazyInitAndUpdate<T>& emplace(TArgs&&... args) {
            child = T(std::forward<TArgs>(args)...);
            return *this;
        }

        constexpr operator T const&() const {
            return child;
        }

        constexpr operator T&() {
            return child;
        }

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            if (this->ctx && &ctx != this->ctx) {
                fmtLog(Logging::Level::ERROR, "Rendered this comp again with a different render ctx. Why? Make a new instance");
            }
            this->ctx = &ctx;
            return QUC::detail::renderSingle(child, ctx);
        }

        constexpr auto update() {
            CRASH_UNLESS(ctx);
            return QUC::detail::renderSingle<T>(child, *ctx);
        }
    };

    template<typename T>
    struct ConditionalRender : public LazyInitAndUpdate<T> {
        using Parent = LazyInitAndUpdate<T>;

        ConditionalRender() = default;

        ConditionalRender(ConditionalRender const&) = delete;
        ConditionalRender(T child) : Parent(std::move(child)) {}

        template<typename... TArgs>
        ConditionalRender(TArgs&&... args) : Parent(std::forward<TArgs>(args)...) {}

        ConditionalRender& operator =(ConditionalRender const&) = default;
        ConditionalRender& operator =(ConditionalRender&&) noexcept = default;

        QUC::RenderHeldData<bool> renderedAllowed = true;

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            if (this->ctx && &ctx != this->ctx) {
                fmtLog(Logging::Level::ERROR,
                       "Rendered this comp again with a different render ctx. Why? Make a new instance");
            }
            this->ctx = &ctx;

            // if modified
            if (renderedAllowed.readAndClear<true>(ctx) && !renderedAllowed.getData()) {
                // hide object

                getLogger().debug("Attempting to destroy");
                auto& context = ctx.getChildDataOrCreate(Parent::child.key);
                if (context.childContext) {
                    UnityEngine::Object::Destroy(&context.childContext->parentTransform);
                    getLogger().debug("Destroying object");
                }

                ctx.destroyChild(Parent::child.key);
            }

            if (!renderedAllowed.getData()) {
                return std::invoke_result_t<decltype(&Parent::update), Parent *>();
            } else {
                return Parent::render(ctx, data);
            }
        }
    };

    // TODO: Move this to QUC proper?
    struct LoadingIndicator {
        const QUC::Key key;

        QUC::RenderHeldData<bool> enabled;

        LoadingIndicator(bool enabled = true) : enabled(enabled) {}

        auto buildLoadingIndicator(UnityEngine::Transform* parent) {
            static QuestUI::WeakPtrGO<UnityEngine::GameObject> loadingTemplate;

            if (!loadingTemplate)
                loadingTemplate = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::GameObject*>().First([](auto& x) { return x->get_name() == "LoadingIndicator"; });
            UnityEngine::GameObject* loadingIndicator = UnityEngine::Object::Instantiate(loadingTemplate.getInner(), parent, false);
            loadingIndicator->set_name("BSMLLoadingIndicator");

            loadingIndicator->AddComponent<UnityEngine::UI::LayoutElement*>();

            return loadingIndicator;
        }

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            auto& loadingIndicator = data.getData<UnityEngine::GameObject*>();

            if (!loadingIndicator) {
                loadingIndicator = buildLoadingIndicator(&ctx.parentTransform);
            }

            if (enabled.readAndClear(ctx)) {
                loadingIndicator->set_active(enabled.getData());
            }

            // create context so it can be yeeted
            return &data.getChildContext([loadingIndicator]{ return loadingIndicator->get_transform();}).parentTransform;
        }
    };

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongListVerticalLayoutGroup(TArgs&&... layoutArgs) {
        QUC::detail::VerticalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        ModifyLayout modify(layout);
        modify.childControlWidth = true;
        modify.childControlHeight = false;
        modify.childForceExpandHeight = false;
        modify.childForceExpandWidth = false;

        ModifyLayoutElement layoutElement(modify);
        layoutElement.preferredWidth = 120;

        return layoutElement;
    }

    template<size_t sz, typename Container = std::array<std::string, sz>, bool copySelfOnCallback = true>
    struct SongListDropDown : public QUC::DropdownSetting<sz, Container, copySelfOnCallback> {
            using Parent = QUC::DropdownSetting<sz, Container, copySelfOnCallback>;

        template<class F>
        constexpr SongListDropDown(std::string_view txt,
                                   std::string_view current, F &&callable,
                                   Container v = Container(), bool enabled_ = true, bool interact = true) : Parent(txt, current, callable, v, enabled_, interact) {}

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            auto ret = Parent::render(ctx, data);
            auto& settingData = data.getData<typename Parent::RenderDropdownData>();
            auto& dropdown = settingData.dropdown;

            dropdown->numberOfVisibleCells = 9;
            dropdown->ReloadData();

            return ret;
        }
    };

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongListHorizontalFilterBar(TArgs&&... layoutArgs) {
        QUC::detail::HorizontalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        ModifyLayout modify(layout);
        modify.childControlWidth = true;
        modify.childControlHeight = true;
        modify.childForceExpandHeight = false;
        modify.childForceExpandWidth = false;
        modify.childScaleWidth = false;

        ModifyLayoutElement layoutElement(modify);
        layoutElement.preferredHeight = 10;

        return layoutElement;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongListHorizontalLayout(TArgs&&... layoutArgs) {
        QUC::detail::HorizontalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        ModifyLayoutElement layoutElement(layout);
        layoutElement.preferredHeight = 70;
        layoutElement.preferredWidth = 80;

        return layoutElement;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongVerticalGroup(TArgs&&... layoutArgs) {
        QUC::detail::VerticalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        ModifyLayout modify(layout);
        modify.padding = {1,1,1,2};

        ModifyContentSizeFitter fitter(layout);
        fitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained;


        QUC::Backgroundable background("round-rect-panel", false, fitter, 1.0f);

        return background;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongHorizontalTitleTimeText(TArgs&&... layoutArgs) {
        QUC::detail::HorizontalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        ModifyLayoutElement layoutElement(layout);
        layoutElement.spacing = 3;

        return layoutElement;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongHorizontalAuthorRatingText(TArgs&&... layoutArgs) {
        QUC::detail::HorizontalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        ModifyLayoutElement layoutElement(layout);
        layoutElement.spacing = 2;
        layoutElement.padding = {0,0,1,2};

        return layoutElement;
    }


    struct CommonSmallText : public QUC::Text {
        CommonSmallText(std::string_view t, float fontSize) : Text(t, true, std::nullopt, fontSize) {}

        constexpr auto render(QUC::RenderContext &ctx, QUC::RenderContextChildData &data) {
            auto ret = QUC::Text::render(ctx, data);
            auto& textComp = data.getData<TMPro::TextMeshProUGUI*>();

            textComp->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
            textComp->set_enableWordWrapping(false);
            textComp->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);

            return ret;
        }
    };

    inline auto TitleTimeText(std::string_view t) {
        return CommonSmallText(t, 2.7f);
    }

    inline auto AuthorRatingText(std::string_view t) {
        return CommonSmallText(t, 2.4f);
    }
}