#pragma once

#include <filesystem>

namespace pce {
using AbsolutePath = std::filesystem::path;
using RelativePath = std::filesystem::path;
using AssetPath = std::filesystem::path;

inline AbsolutePath Absolute(const RelativePath& relative_path) { return absolute(relative_path); }
inline AbsolutePath Asset(const AssetPath& asset_path) {
    const RelativePath assets_dir = "assets";
    return absolute(assets_dir / asset_path);
}
} // namespace pce
