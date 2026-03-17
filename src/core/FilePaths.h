#pragma once

#include <filesystem>

namespace FilePaths {

std::filesystem::path resolveExistingPath(const std::filesystem::path &path);

} // namespace FilePaths
