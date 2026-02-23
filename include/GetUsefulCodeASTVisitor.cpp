#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

#include <utility>
#include <string>

#include "GetUsefulCodeASTVisitor.h"

using Self = GetUsefulCodeASTVisitor;
using Base = clang::RecursiveASTVisitor<Self>;

bool Self::VisitDecl(clang::Decl* D) {
    if (!D || context->getSourceManager().isInSystemHeader(D->getLocation())) return true;

    if (auto* UD = clang::dyn_cast<clang::UsingDecl>(D)) {
        for (auto* USD : UD->shadows()) {
            if (valid_decl.contains(USD->getTargetDecl()->getCanonicalDecl())) {
                if (auto Code = getSourceCodeFromDecl(D)) {
                    auto& Ctx = D->getASTContext();
                    auto& SM = Ctx.getSourceManager();
                    auto FID = SM.getFileID(D->getLocation());
                    auto OptionalFileEntry = SM.getFileEntryRefForID(FID);
                    if (!OptionalFileEntry) break;
                    auto FileEntry = *OptionalFileEntry;

                    valid_codes.emplace_back(FileEntry.getUniqueID(), SM.getFileOffset(D->getLocation()), Code.value().append(";\n"));
                }
            }

            break;
        }
    } else if (valid_decl.contains(D->getCanonicalDecl())) {
        auto Code = getSourceCodeFromDecl(D);
        if (Code.has_value() && !visited_code.contains(*Code)) {
            visited_code.insert(*Code);

            auto& Ctx = D->getASTContext();
            auto& SM = Ctx.getSourceManager();
            auto FID = SM.getFileID(D->getLocation());
            auto OptionalFileEntry = SM.getFileEntryRefForID(FID);
            if (!OptionalFileEntry) return true;
            auto FileEntry = *OptionalFileEntry;

            valid_codes.emplace_back(FileEntry.getUniqueID(), SM.getFileOffset(D->getLocation()), Code.value().append(";\n"));
        }
    }

    return true;
}

std::optional<std::string> Self::getSourceCodeFromDecl(const clang::Decl* D) {
    if (!D) return std::nullopt;

    auto& Ctx = D->getASTContext();
    auto& SM = Ctx.getSourceManager();
    auto& LangOpts = Ctx.getLangOpts();

    auto Begin = D->getBeginLoc(), End = D->getEndLoc();
    if (Begin.isInvalid() || End.isInvalid()) return std::nullopt;
    Begin = SM.getSpellingLoc(Begin), End = SM.getSpellingLoc(End);
    if (Begin.isInvalid() || End.isInvalid()) return std::nullopt;

    auto CharSourceRange = clang::CharSourceRange::getTokenRange(Begin, End);

    auto CodeRef = clang::Lexer::getSourceText(CharSourceRange, SM, LangOpts);
    if (CodeRef.empty()) return std::nullopt;
    return CodeRef.str();
}