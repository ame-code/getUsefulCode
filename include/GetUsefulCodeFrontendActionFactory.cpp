#include "GetUsefulCodeFrontendActionFactory.h"
#include "GetUsefulCodeASTAction.h"

using Self = GetUsefulCodeFrontendActionFactory;

std::unique_ptr<clang::FrontendAction> Self::create() {
    return std::make_unique<GetUsefulCodeASTAction>(ValidCode);
}