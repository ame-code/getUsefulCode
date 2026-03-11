#include <llvm/Support/CommandLine.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <iostream>
#include <filesystem>

#include <GetUsefulCodeFrontendActionFactory.h>
#include <MyCompilationDataBase.h>

namespace fs = std::filesystem;

#include <bit>

int main(int argc, char** argv) {
    std::bit_width(10u);
    assert(argc > 1 && "need args");
    assert(fs::exists(argv[1]) && "File not exist");
    
    MyCompilationDataBase DB;
    std::vector<std::string> SourcePathList{argv[1]};
    clang::tooling::ClangTool Tool(DB,SourcePathList);

    std::string code;
    GetUsefulCodeFrontendActionFactory Factory(code);
    auto result = Tool.run(&Factory);
    std::cout << code << '\n';
    return result;
}