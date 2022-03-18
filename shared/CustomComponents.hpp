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
#include "questui_components/shared/components/Modal.hpp"
#include "HMUI/Touchable.hpp"

#include "main.hpp"

namespace BetterSongSearch::UI {

    // Allows for updating without providing RenderCtx
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

        constexpr T const* operator ->() const {
            return &child;
        }

        constexpr T* operator ->() {
            return &child;
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
            if (renderedAllowed.readAndClear(ctx) && !renderedAllowed.getData()) {
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

    template<typename T>
    struct HideObject : public T {
        QUC::RenderHeldData<bool> active = true;

        HideObject() = default;
        HideObject(HideObject const&) = default;

        HideObject(T child, bool active = true) : T(std::move(child)), active(active) {}

        template<typename... TArgs>
        HideObject(TArgs&&... args) : T(std::forward<TArgs>(args)...) {}

        constexpr auto render(QUC::RenderContext& ctx, QUC::RenderContextChildData& data) {
            auto ret = T::render(ctx, data);

            if (active.readAndClear<QUC::DiffModifyCheck::TRUE_WHEN_FOUND_OR_ASSIGNED>(ctx)) {
                ret->get_gameObject()->SetActive(*active);
            }
        }
    };

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongListVerticalLayoutGroup(TArgs&&... layoutArgs) {
        QUC::detail::VerticalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        layout.childControlWidth = true;
        layout.childControlHeight = false;
        layout.childForceExpandHeight = false;
        layout.childForceExpandWidth = false;

        QUC::ModifyLayoutElement layoutElement(layout);
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
        layout.childControlWidth = true;
        layout.childControlHeight = true;
        layout.childForceExpandHeight = false;
        layout.childForceExpandWidth = false;
        layout.childScaleWidth = false;

        QUC::ModifyLayoutElement layoutElement(layout);
        layoutElement.preferredHeight = 10;

        return layoutElement;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongListHorizontalLayout(TArgs&&... layoutArgs) {
        QUC::detail::HorizontalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        QUC::ModifyLayoutElement layoutElement(layout);
        layoutElement.preferredHeight = 70;
        layoutElement.preferredWidth = 80;

        return layoutElement;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongVerticalGroup(TArgs&&... layoutArgs) {
        QUC::detail::VerticalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        layout.padding = {1,1,1,2};

        QUC::ModifyContentSizeFitter fitter(layout);
        fitter.horizontalFit = UnityEngine::UI::ContentSizeFitter::FitMode::Unconstrained;


        QUC::Backgroundable background("round-rect-panel", false, fitter, 1.0f);

        return background;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongHorizontalTitleTimeText(TArgs&&... layoutArgs) {
        QUC::detail::HorizontalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        QUC::ModifyLayoutElement layoutElement(layout);
        layoutElement.spacing = 3;

        return layoutElement;
    }

    template<class... TArgs>
    requires ((QUC::renderable<TArgs> && ...))
    constexpr auto SongHorizontalAuthorRatingText(TArgs&&... layoutArgs) {
        QUC::detail::HorizontalLayoutGroup<TArgs...> layout(std::forward<TArgs>(layoutArgs)...);
        QUC::ModifyLayoutElement layoutElement(layout);
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