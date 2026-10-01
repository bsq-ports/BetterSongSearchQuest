#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace BetterSongSearch::Util {
    struct SongHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view hash) const noexcept {
            return std::hash<std::string_view>{}(hash);
        }
    };

    using SongScores = std::unordered_set<std::string, SongHash, std::equal_to<>>;
}
