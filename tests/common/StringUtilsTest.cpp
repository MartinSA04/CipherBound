#include "../../src/common/StringUtils.h"
#include <cassert>
#include <string>
#include <vector>

int main() {
    std::string trimmed = "value \r\t";
    StringUtils::trimRightInPlace(trimmed);
    assert(trimmed == "value");

    const std::vector<std::string> pipeParts = StringUtils::splitPipe("a|b|c");
    assert((pipeParts == std::vector<std::string>{"a", "b", "c"}));

    const std::vector<std::string> semicolonParts = StringUtils::splitSemicolon("one;two");
    assert((semicolonParts == std::vector<std::string>{"one", "two"}));
    assert(StringUtils::splitSemicolon("-").empty());

    const std::vector<std::string> doubleAtParts =
        StringUtils::splitDoubleAt("first@@second@@third");
    assert((doubleAtParts == std::vector<std::string>{"first", "second", "third"}));

    assert(StringUtils::replaceAll("alpha beta alpha", "alpha", "gamma") == "gamma beta gamma");
    assert(StringUtils::substitutePlayerName("Hello, {player}!", "Ada") == "Hello, Ada!");
    assert(StringUtils::substitutePlayerName("Hello, {player_name}!", "Ada") ==
           "Hello, Ada!");
    assert((StringUtils::substitutePlayerName(std::vector<std::string>{"{player}", "{player_name}"},
                                              "Ada") ==
            std::vector<std::string>{"Ada", "Ada"}));

    assert(StringUtils::parseDirection("up") == Direction::up);
    assert(StringUtils::parseDirection("right") == Direction::right);
    assert(StringUtils::capitalize("daemon") == "Daemon");
    assert(StringUtils::capitalize("") == "");

    return 0;
}
