#include "MyCompilationDataBase.h"

#include <filesystem>

using Self = MyCompilationDataBase;

namespace fs = std::filesystem;

const char* llvm_dir = getenv("LLVM_DIR");
const char* home = getenv("HOME");

std::vector<clang::tooling::CompileCommand> Self::getCompileCommands(llvm::StringRef FilePath) const
{
    fs::path clang_path = config_data_.llvm_path / "bin" / "clang++";
    fs::path clang_resource_path = config_data_.llvm_path / "lib" / "clang";
    {
        fs::path tmp;
        for (auto it = fs::directory_iterator(clang_resource_path), e = fs::directory_iterator(); it != e; ++it) {
            if (!fs::exists(tmp)) {
                tmp = *it;
            } else if (fs::last_write_time(tmp) < it->last_write_time()) {
                tmp = *it;
            }
        }
        clang_resource_path = tmp;
    }
    std::vector<std::string> command_line = {
        clang_path.string(),
        // "-E",
        // "-v",
        FilePath.str(),
        std::format("-resource-dir={}", clang_resource_path.string()),
        "-std=c++23",
        "-DREED",
    };
    for (auto& include_dir : config_data_.include_paths) {
        command_line.emplace_back(std::format("-I{}", std::move(include_dir).string()));
    }
    return {
        clang::tooling::CompileCommand(
            fs::path(FilePath.str()).parent_path().string(),
            FilePath.str(),
            std::move(command_line),
            FilePath.str()
    )};

    // assert(llvm_dir != nullptr && "llvm_dir must be non-null");
    // assert(home != nullptr && "home must be non-null");
    // fs::path clang_path = fs::path(llvm_dir) / "bin" / "clang++";
    // fs::path clang_lib_path = fs::path(llvm_dir) / "lib" / "clang";
    // fs::path clang_resource_dir;
    // for (auto it = fs::directory_iterator(clang_lib_path), e = fs::directory_iterator(); it != e; ++it)
    // {
    //     clang_resource_dir = *it;
    // }
    // fs::path local_include_dir = fs::path(home) / ".local" / "include";
    // return {
    //     clang::tooling::CompileCommand(
    //         fs::path(FilePath.str()).parent_path().string(),
    //         FilePath.str(),
    //         {
    //             clang_path.string(),
    //             // "-E",
    //             // "-v",
    //             "-resource-dir=" + clang_resource_dir.string(),
    //             "-I" + local_include_dir.string(),
    //             "-std=c++23",
    //             "-DREED",
    //             FilePath.str()
    //         },
    //         FilePath.str()
    //     )
    // };
}
