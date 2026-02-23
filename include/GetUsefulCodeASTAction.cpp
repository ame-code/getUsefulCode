#include "GetUsefulCodeASTAction.h"
#include "RecordPPCallbacks.h"
#include "GetUsefulCodeASTConsumer.h"

using Self = GetUsefulCodeASTAction;
using Base = clang::ASTFrontendAction;

bool Self::BeginSourceFileAction(clang::CompilerInstance &CI) {
    auto& PP = CI.getPreprocessor();
    auto& SM = CI.getSourceManager();

    PP.addPPCallbacks(
        std::make_unique<RecordPPCallbacks>(SM, CI.getLangOpts(), codes, edges)
    );

    return true;
}

std::unique_ptr<clang::ASTConsumer> Self::CreateASTConsumer(clang::CompilerInstance& CI, clang::StringRef InFile) {
    return std::make_unique<GetUsefulCodeASTConsumer>(&CI.getASTContext(), codes, edges, ValidCode);
}