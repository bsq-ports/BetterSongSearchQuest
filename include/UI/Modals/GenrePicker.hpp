#pragma once

#include "HMUI/ViewController.hpp"
#include "bsml/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "HMUI/ModalView.hpp"
#include "HMUI/TableView.hpp"
#include "bsml/shared/BSML/Components/Settings/SliderSetting.hpp"
#include "bsml/shared/BSML/Components/ModalView.hpp"
#include "bsml/shared/BSML/ViewControllers/HotReloadViewController.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "custom-types/shared/macros.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "bsml/shared/BSML/Components/CustomListTableData.hpp"


DECLARE_CLASS_CODEGEN(BetterSongSearch::UI::Modals, GenrePicker, UnityEngine::MonoBehaviour,
    DECLARE_INSTANCE_METHOD(void, OnEnable);
    DECLARE_CTOR(ctor);

    DECLARE_INSTANCE_METHOD(void, PostParse);

    DECLARE_INSTANCE_METHOD(void, OpenModal);
    DECLARE_INSTANCE_METHOD(void, CloseModal);



    DECLARE_INSTANCE_METHOD(void, ClearGenre);

    DECLARE_INSTANCE_FIELD(bool, initialized);

    // Modals
    DECLARE_INSTANCE_FIELD(UnityW<BSML::ModalView>, genrePickerModal);

    public: 
        std::vector<std::string> selectedGenres;
        std::vector<std::string> excludedGenres;

)
