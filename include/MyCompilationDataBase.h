#pragma once

#include <clang/Tooling/CompilationDatabase.h>

#include <vector>
#include <string>

class MyCompilationDataBase : public clang::tooling::CompilationDatabase {
public:
    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef) const override;
};