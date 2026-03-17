#pragma once

#include <clang/Tooling/CompilationDatabase.h>

#include <vector>
#include <string>

#include "ConfigData.hpp"

class MyCompilationDataBase final : public clang::tooling::CompilationDatabase {
    ConfigData config_data_;
public:
    explicit MyCompilationDataBase(ConfigData config_data): config_data_(std::move(config_data)) {}

    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef) const override;
};