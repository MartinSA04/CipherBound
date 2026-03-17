#include "../../src/common/TextParse.h"
#include <cassert>
#include <string_view>
#include <vector>

int main() {
    const auto parts = TextParse::splitView("a|b|c", '|');
    assert(parts.size() == 3);
    assert(parts[0] == "a");
    assert(parts[2] == "c");

    const auto keyVal = TextParse::splitOnce("name|Cipher", '|');
    assert(keyVal.has_value());
    assert(keyVal->first == "name");
    assert(keyVal->second == "Cipher");

    assert(TextParse::parseInt("42") == 42);
    assert(TextParse::parseInt("-7") == -7);
    assert(!TextParse::parseInt("7x").has_value());

    const auto coords = TextParse::parseFixedIntList<2>("10|-4", '|');
    assert(coords.has_value());
    assert((*coords)[0] == 10);
    assert((*coords)[1] == -4);

    const std::vector<std::string_view> fields = {"warp", "3", "5", "route_1"};
    const auto values = TextParse::parseFixedIntFields<2>(fields, 1);
    assert(values.has_value());
    assert((*values)[0] == 3);
    assert((*values)[1] == 5);

    return 0;
}
