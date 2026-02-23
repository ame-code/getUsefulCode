#include "MyCompilationDataBase.h"

#include <filesystem>

using Self = MyCompilationDataBase;

namespace fs = std::filesystem;

std::vector<clang::tooling::CompileCommand> Self::getCompileCommands(llvm::StringRef FilePath) const {
    return {clang::tooling::CompileCommand(
        fs::path(FilePath.str()).parent_path().string(),
        FilePath.str(),
        {
            "/opt/llvm-tooling/bin/clang++",
            // "-E",
            // "-v",
            "-resource-dir=/opt/llvm-tooling/lib/clang/22",
            "-I/home/reed/.local/include",
            "-std=c++23",
            "-DREED",
            FilePath.str()
        },
        FilePath.str()
    )};
}