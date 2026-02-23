#pragma once

#include <clang/AST/RecursiveASTVisitor.h>

#include <unordered_set>
#include <utility>
#include <string>

#include "MyClangTools.h"

class GetUsefulCodeASTVisitor : public clang::RecursiveASTVisitor<GetUsefulCodeASTVisitor> {
    clang::ASTContext* context;
    std::unordered_set<clang::Decl*> valid_decl;
    std::unordered_set<std::string> visited_code;
    std::vector<CodeInfo> valid_codes;

public:
    GetUsefulCodeASTVisitor(clang::ASTContext* context): context(context) {}

    GetUsefulCodeASTVisitor& setValidDecl(const std::unordered_set<clang::Decl*>& valid_decl) {
        this->valid_decl = valid_decl;
        return *this;
    }
    GetUsefulCodeASTVisitor& setValidDecl(std::unordered_set<clang::Decl*>&& valid_decl) {
        this->valid_decl = std::move(valid_decl);
        return *this;
    }
    auto& getValidCodes() & {
        return valid_codes;
    }
    const auto& getValidCodes() const & {
        return valid_codes;
    }
    auto&& getValidCodes() && {
        return std::move(valid_codes);
    }
    const auto&& getValidCodes() const && {
        return std::move(valid_codes);
    }

    bool VisitDecl(clang::Decl*);

private:
    std::optional<std::string> getSourceCodeFromDecl(const clang::Decl*);
};