#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

#include <utility>
#include <string>

#include "GetUsefulCodeASTVisitor.h"

#include <filesystem>

using Self = GetUsefulCodeASTVisitor;
using Base = clang::RecursiveASTVisitor<Self>;

bool Self::VisitDecl(clang::Decl* D) {
    if (!D || context.getSourceManager().isInSystemHeader(D->getLocation())) return true;

    if (!valid_decl.contains(D->getCanonicalDecl())) return true;

    auto Code = tryGetCodeFromDecl(D);
    if (!Code) return true;

    auto UniqueID = tryGetUniqueID(D);
    if (!UniqueID.has_value()) return true;

    valid_codes.emplace_back(*UniqueID, getFileOffset(D), std::move(*Code).append(";\n"));

    return true;
}

std::optional<std::string> Self::getSourceCodeFromDecl(const clang::Decl* D) const {
    if (!D) return std::nullopt;

    auto& LangOpts = context.getLangOpts();

    const auto range = D->getSourceRange();

    bool invalid = false;

    const auto CodeRef = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range.getBegin(), range.getEnd()), SM, LangOpts, &invalid);

    if (invalid) {
        return std::nullopt;
    }
    return CodeRef.str();
}

std::optional<std::string> Self::tryGetCodeFromDecl(clang::Decl *D) {
    if (const auto* UD = clang::dyn_cast<clang::UsingDecl>(D)) {
        return tryGetCodeFromUsingDecl(UD);
    }
    if (const auto* ND = clang::dyn_cast<clang::NamespaceDecl>(D)) {
        return tryGetCodeFromNamespaceDecl(ND);
    }
    if (const auto* VTSD = clang::dyn_cast<clang::VarTemplateSpecializationDecl>(D)) {
        if (VTSD->isExplicitSpecialization()) {
            return tryGetCodeFromDecl(D);
        } else {
            return std::nullopt;
        }
    }
    return getSourceCodeFromDecl(D);
}

std::optional<std::string> Self::tryGetCodeFromUsingDecl(const clang::UsingDecl *UD) {
    for (const auto* USD : UD->shadows()) {
        if (!valid_decl.contains(USD->getTargetDecl()->getCanonicalDecl())) continue;
        return getSourceCodeFromDecl(UD);
    }
    return std::nullopt;
}

std::optional<std::string> Self::tryGetCodeFromNamespaceDecl(const clang::NamespaceDecl *ND) {
    std::string codes;
    bool invalid = true;
    for (auto* D : ND->decls()) {
        if (!valid_decl.contains(D->getCanonicalDecl())) continue;

        auto Code = tryGetCodeFromDecl(D);
        if (!Code) continue;

        invalid = false;

        codes.append(std::move(*Code).append(";\n"));
    }

    if (invalid) return std::nullopt;

    codes = std::format("namespace {} {{\n{}}}", ND->getName().str(), std::move(codes));
    return codes;
}


std::optional<llvm::sys::fs::UniqueID> Self::tryGetUniqueID(const clang::Decl *D) {
    auto& Ctx = D->getASTContext();
    const auto& SM = Ctx.getSourceManager();
    auto loc = D->getLocation();
    if (loc.isMacroID()) {
        loc = SM.getExpansionLoc(loc);
    }
    const auto FID = SM.getFileID(loc);
    const auto FileEntryPtr = SM.getFileEntryForID(FID);
    if (!FileEntryPtr) return std::nullopt;
    return FileEntryPtr->getUniqueID();
}

unsigned Self::getFileOffset(const clang::Decl *D) const {
    auto loc = D->getLocation();
    if (loc.isMacroID()) {
        loc = SM.getExpansionLoc(loc);
    }
    return SM.getFileOffset(loc);
}
