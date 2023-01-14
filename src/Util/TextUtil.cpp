#include "Util/TextUtil.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <cctype> 
namespace BetterSongSearch::Util { 
    // this hurts
    std::vector<std::string> split(std::string_view buffer, const std::string_view delimeter) {
        std::vector<std::string> ret{};
        std::decay_t<decltype(std::string::npos)> pos{};
        while ((pos = buffer.find(delimeter)) != std::string::npos) {
            const auto match = buffer.substr(0, pos);
            if (!match.empty()) ret.emplace_back(match);
            buffer = buffer.substr(pos + delimeter.size());
        }
        if (!buffer.empty()) ret.emplace_back(buffer);
        return ret;
    }

    /**
     * Removes special characters from a string
    */
    std::string removeSpecialCharacter(std::string_view const s) {
        std::string stringy(s);
        for (int i = 0; i < stringy.size(); i++) {

            if (stringy[i] != ' ' && 
                stringy[i] < 'A' || stringy[i] > 'Z' &&
                stringy[i] < 'a' || stringy[i] > 'z' )
            {
                stringy.erase(i, 1);
                i--;
            }
        }
        return stringy;
    }

    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    std::string toLower(std::string_view s) {
        return toLower(std::string(s));
    }

    std::string toLower(char const* s) {
        return toLower(std::string(s));
    }

    bool IsSpace(char x) { return x == ' ' || !isalnum(x); };
}
