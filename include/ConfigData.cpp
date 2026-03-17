//
// Created by reed on 2026/3/16.
//

#include <filesystem>

#include "ConfigData.hpp"

#include <format>
#include <iomanip>

ConfigData ConfigData::makeConfigData(const nlohmann::json &json) {
    assert(json.is_object() && "json format error, must be object");

    const auto& llvm_dir_str = json["llvm dir"];
    assert(llvm_dir_str.is_string() && "llvm dir must be string");

    const auto& include_dir_arr = json["include dirs"];
    assert(include_dir_arr.is_array() && "include dirs must be array");

    ConfigData data;
    data.llvm_path = std::filesystem::path(llvm_dir_str);
    for (const auto& include_dir_str : include_dir_arr) {
        data.include_paths.emplace_back(include_dir_str);
    }

    return data;
}
