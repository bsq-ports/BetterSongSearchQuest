#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace BetterSongSearch::Util {
    class SearchTextCache {
       public:
        struct Text {
            std::string_view songName;
            std::string_view songAuthorName;
            std::string_view levelAuthorName;
        };

        explicit SearchTextCache(uint64_t generation) : generation(generation) {}

        uint64_t const generation;

        void Reserve(std::size_t songs, std::size_t characters) {
            if (characters > std::numeric_limits<uint32_t>::max()) {
                throw std::length_error("Normalized search text exceeds 4 GiB");
            }
            _offsets.reserve(songs);
            _characters.reserve(characters);
        }

        void Append(std::string_view songName, std::string_view songAuthorName, std::string_view levelAuthorName) {
            Offsets offsets;
            offsets.songName = _characters.size();
            _characters.append(songName);
            offsets.songAuthorName = _characters.size();
            _characters.append(songAuthorName);
            offsets.levelAuthorName = _characters.size();
            _characters.append(levelAuthorName);
            offsets.end = _characters.size();
            _offsets.push_back(offsets);
        }

        Text Get(std::size_t index) const {
            auto const& offsets = _offsets[index];
            auto characters = _characters.data();
            return {
                {characters + offsets.songName, offsets.songAuthorName - offsets.songName},
                {characters + offsets.songAuthorName, offsets.levelAuthorName - offsets.songAuthorName},
                {characters + offsets.levelAuthorName, offsets.end - offsets.levelAuthorName}
            };
        }

        std::size_t SongCount() const { return _offsets.size(); }
        std::size_t IndexBytes() const { return _offsets.capacity() * sizeof(Offsets); }
        std::size_t TextBytes() const { return _characters.capacity() + 1; }
        std::size_t MemoryBytes() const { return sizeof(*this) + IndexBytes() + TextBytes(); }

       private:
        struct Offsets {
            uint32_t songName;
            uint32_t songAuthorName;
            uint32_t levelAuthorName;
            uint32_t end;
        };
        static_assert(sizeof(Offsets) == 16);

        // Offsets stay valid while the character buffer grows; published caches are immutable.
        std::vector<Offsets> _offsets;
        std::string _characters;
    };
}
