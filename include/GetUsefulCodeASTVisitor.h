#pragma once

#include <clang/AST/RecursiveASTVisitor.h>

#include <unordered_set>
#include <utility>
#include <string>

#include "MyClangTools.h"

class GetUsefulCodeASTVisitor : public clang::RecursiveASTVisitor<GetUsefulCodeASTVisitor> {
    clang::ASTContext& context;
    const clang::SourceManager& SM;
    std::unordered_set<clang::Decl*> valid_decl;
    std::unordered_set<std::string> visited_code;
    std::vector<CodeInfo> valid_codes;

public:
    GetUsefulCodeASTVisitor(clang::ASTContext* context): context(*context), SM(context->getSourceManager()) {}

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
    void dispatchDecl(clang::Decl* D);

    std::optional<std::string> tryGetCodeFromUsingDecl(const clang::UsingDecl* UD);
    std::optional<std::string> tryGetCodeFromNamespaceDecl(const clang::NamespaceDecl* ND);
    std::optional<std::string> tryGetCodeFromDecl(clang::Decl* D);

    void handleUsingDecl(clang::UsingDecl* UD);
    void handleNamespaceDecl(clang::NamespaceDecl* ND);
    void handleDecl(clang::Decl* D);

    static std::optional<llvm::sys::fs::UniqueID> tryGetUniqueID(const clang::Decl* D);
    unsigned getFileOffset(const clang::Decl* D) const;

    std::optional<std::string> getSourceCodeFromDecl(const clang::Decl*) const;
};