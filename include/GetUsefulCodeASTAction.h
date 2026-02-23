#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>

#include <memory>
#include <vector>
#include <string>

#include "MyClangTools.h"

class GetUsefulCodeASTAction : public clang::ASTFrontendAction {
    std::vector<CodeInfo> codes;
    std::vector<Edge> edges;
    std::string& ValidCode;
public:
    GetUsefulCodeASTAction(std::string& ValidCode): ValidCode(ValidCode) {}

    bool BeginSourceFileAction(clang::CompilerInstance &CI) override;
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI, clang::StringRef InFile) override;
};