#pragma once

#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendAction.h>

#include <memory>
#include <string>

class GetUsefulCodeFrontendActionFactory : public clang::tooling::FrontendActionFactory {
    std::string& ValidCode;

public:
    GetUsefulCodeFrontendActionFactory(std::string& ValidCode): ValidCode(ValidCode) {}

    std::unique_ptr<clang::FrontendAction> create() override;
};