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

    // if (auto* UD = clang::dyn_cast<clang::UsingDecl>(D)) {
    //     for (auto* USD : UD->shadows()) {
    //         if (valid_decl.contains(USD->getTargetDecl()->getCanonicalDecl())) {
    //             if (auto Code = getSourceCodeFromDecl(D)) {
    //                 auto& Ctx = D->getASTContext();
    //                 auto& SM = Ctx.getSourceManager();
    //                 auto FID = SM.getFileID(D->getLocation());
    //                 auto OptionalFileEntry = SM.getFileEntryRefForID(FID);
    //                 if (!OptionalFileEntry) break;
    //                 auto FileEntry = *OptionalFileEntry;
    //
    //                 valid_codes.emplace_back(FileEntry.getUniqueID(), SM.getFileOffset(D->getLocation()), Code.value().append(";\n"));
    //             }
    //         }
    //
    //         break;
    //     }
    // } else if (auto* ND = clang::dyn_cast<clang::NamespaceDecl>(D)) {
    //     std::string codes;
    //     for (const auto decl : ND->decls()) {
    //         auto Code = getSourceCodeFromDecl(decl);
    //         if (Code.has_value() && !visited_code.contains(Code.value())) {
    //             visited_code.insert(Code.value());
    //
    //             codes.append(Code.value().append(";\n"));
    //         }
    //     }
    //     if (codes.empty()) return true;
    //
    //     auto& Ctx = D->getASTContext();
    //     auto& SM = Ctx.getSourceManager();
    //     auto loc = D->getLocation();
    //     if (loc.isMacroID()) {
    //         loc = SM.getExpansionLoc(loc);
    //     }
    //     auto FID = SM.getFileID(loc);
    //     auto FileEntryPtr = SM.getFileEntryForID(FID);
    //     if (!FileEntryPtr) { return true; }
    //
    //     codes = std::format("namespace {} {{\n{}\n}}\n", ND->getName().str(), codes);
    //
    // } else if (valid_decl.contains(D->getCanonicalDecl())) {
    //     auto Code = getSourceCodeFromDecl(D);
    //     if (Code.has_value() && !visited_code.contains(*Code)) {
    //         visited_code.insert(*Code);
    //
    //         auto& Ctx = D->getASTContext();
    //         auto& SM = Ctx.getSourceManager();
    //         auto loc = D->getLocation();
    //         if (loc.isMacroID()) {
    //             loc = SM.getExpansionLoc(loc);
    //         }
    //         auto FID = SM.getFileID(loc);
    //         auto FileEntryPtr = SM.getFileEntryForID(FID);
    //         if (!FileEntryPtr) { return true; }
    //
    //         valid_codes.emplace_back(FileEntryPtr->getUniqueID(), SM.getFileOffset(loc), Code.value().append(";\n"));
    //     }
    // }

    if (!valid_decl.contains(D->getCanonicalDecl())) return true;

    auto Code = tryGetCodeFromDecl(D);
    if (!Code || visited_code.contains(*Code)) return true;

    auto UniqueID = tryGetUniqueID(D);
    if (!UniqueID.has_value()) return true;

    valid_codes.emplace_back(*UniqueID, getFileOffset(D), Code.value().append(";\n"));

    return true;
}

// void Self::dispatchDecl(clang::Decl *D) {
//     if (auto* UD = clang::dyn_cast<clang::UsingDecl>(D)) {
//         handleUsingDecl(UD);
//     } else if (auto* ND = clang::dyn_cast<clang::NamespaceDecl>(D)) {
//         handleNamespaceDecl(ND);
//     } else {
//         handleDecl(D);
//     }
// }

std::optional<std::string> Self::getSourceCodeFromDecl(const clang::Decl* D) {
    if (!D) return std::nullopt;

    auto& LangOpts = context.getLangOpts();

    const auto range = D->getSourceRange();

    const auto begin = SM.getExpansionLoc(range.getBegin());
    const auto end = SM.getExpansionLoc(range.getEnd());

    bool invalid = false;

    const auto CodeRef = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range.getBegin(), range.getEnd()), SM, LangOpts, &invalid);

    if (invalid) {
        return std::nullopt;
    }
    return CodeRef.str();
}

// void Self::handleDecl(clang::Decl *D) {
//     if (auto Code = getSourceCodeFromDecl(D); Code.has_value() && !visited_code.contains(Code.value())) {
//         visited_code.insert(Code.value());
//
//         auto UniqueID = tryGetUniqueID(D);
//         if (!UniqueID.has_value()) return;
//
//         valid_codes.emplace_back(UniqueID.value(), getFileOffset(D), Code.value().append(";\n"));
//     }
// }

std::optional<std::string> Self::tryGetCodeFromDecl(clang::Decl *D) {
    if (const auto* UD = clang::dyn_cast<clang::UsingDecl>(D)) {
        return tryGetCodeFromUsingDecl(UD);
    }
    if (const auto* ND = clang::dyn_cast<clang::NamespaceDecl>(D)) {
        return tryGetCodeFromNamespaceDecl(ND);
    }
    return getSourceCodeFromDecl(D);
}

// void Self::handleUsingDecl(clang::UsingDecl *UD) {
//     auto Code = tryGetCodeFromUsingDecl(UD);
//     if (!Code || visited_code.contains(Code.value())) return;
//
//     auto UniqueID = tryGetUniqueID(UD);
//     if (!UniqueID.has_value()) return;
//
//     valid_codes.emplace_back(*UniqueID, getFileOffset(UD), Code.value().append(";\n"));
// }

std::optional<std::string> Self::tryGetCodeFromUsingDecl(const clang::UsingDecl *UD) {
    for (const auto* USD : UD->shadows()) {
        if (!valid_decl.contains(USD->getTargetDecl()->getCanonicalDecl())) continue;
        auto Code = getSourceCodeFromDecl(UD);
        if (!Code || visited_code.contains(Code.value())) return std::nullopt;

        return Code;
    }
    return std::nullopt;
}

std::optional<std::string> Self::tryGetCodeFromNamespaceDecl(const clang::NamespaceDecl *ND) {
    std::string codes;
    bool invalid = true;
    for (auto* D : ND->decls()) {
        if (!valid_decl.contains(D->getCanonicalDecl())) continue;

        auto Code = tryGetCodeFromDecl(D);
        if (!Code || visited_code.contains(Code.value())) return std::nullopt;

        invalid = false;

        codes.append(Code.value().append(";\n"));
    }

    if (invalid) return std::nullopt;

    codes = std::format("namespace {} {{\n{}}}", ND->getName().str(), codes);
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
