#include "FilePaths.h"

namespace FilePaths {

std::filesystem::path resolveExistingPath(const std::filesystem::path &path) {
    if (path.is_absolute() || path.empty())
        return path;

    if (std::filesystem::exists(path))
        return path;

    std::filesystem::path base = std::filesystem::current_path();
    while (!base.empty()) {
        const std::filesystem::path candidate = base / path;
        if (std::filesystem::exists(candidate))
            return candidate;

        const std::filesystem::path parent = base.parent_path();
        if (parent == base)
            break;
        base = parent;
    }

    return path;
}

} // namespace FilePaths
