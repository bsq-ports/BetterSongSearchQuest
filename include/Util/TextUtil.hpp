#pragma once

#include <vector>
#include <string>
#include <algorithm>

namespace BetterSongSearch::Util { 
    // this hurts
    std::vector<std::string> split(std::string_view buffer, const std::string_view delimeter = " ");

    /**
     * Removes special characters from a string
    */
    std::string removeSpecialCharacter(std::string_view const s);
    std::string toLower(std::string s);
    std::string toLower(std::string_view s);
    std::string toLower(char const* s);

    bool IsSpace(char x);
}
