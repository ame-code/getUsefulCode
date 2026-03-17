#pragma once

#include <filesystem>
#include <vector>

#include <nlohmann/json.hpp>

class ConfigData {
public:
    std::filesystem::path llvm_path;
    std::vector<std::filesystem::path> include_paths;

    static ConfigData makeConfigData(const nlohmann::json& json);
};
