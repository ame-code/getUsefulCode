#include <llvm/Support/CommandLine.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <filesystem>

#include <GetUsefulCodeFrontendActionFactory.h>
#include <MyCompilationDataBase.h>
#include <ConfigData.hpp>
#include <fstream>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    assert(argc > 1 && "need args");
    assert(fs::exists(argv[1]) && "File not exist");

    // std::cerr << argv[0] << '\n';

    fs::path root_path =
    #ifndef NDEBUG
    fs::path(argv[0]).parent_path().parent_path();
    #else
    fs::path(INSTALL_PREFIX);
    #endif
    // std::cerr << root_path << '\n';

    fs::path config_path =
    #ifndef NDEBUG
    root_path / "config.json";
    #else
    root_path / "etc" / "getUsefulCode" / "config.json";
    #endif

    nlohmann::json json;
    std::ifstream ifs(config_path);
    // std::cerr << config_path << '\n';
    assert(ifs.is_open());
    ifs >> json;

    ConfigData data = ConfigData::makeConfigData(json);

    MyCompilationDataBase DB(std::move(data));
    std::vector<std::string> SourcePathList{argv[1]};
    clang::tooling::ClangTool Tool(DB,SourcePathList);

    std::string code;
    GetUsefulCodeFrontendActionFactory Factory(code);
    auto result = Tool.run(&Factory);
    std::cout << code << '\n';
    return result;
}