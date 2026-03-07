#pragma once

#include <clang/Lex/PPCallbacks.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/LangOptions.h>

#include <vector>
#include <set>
#include <utility>
#include <string>

#include "MyClangTools.h"

class RecordPPCallbacks : public clang::PPCallbacks {
    clang::SourceManager& SM;
    const clang::LangOptions& LangOpts;
    std::vector<CodeInfo>& codes;
    std::set<llvm::sys::fs::UniqueID> visited_file;
    std::vector<Edge>& edges;

public:
    RecordPPCallbacks(clang::SourceManager& SM, const clang::LangOptions& LangOpts, std::vector<CodeInfo>& codes, std::vector<Edge>& edges): SM(SM), LangOpts(LangOpts), codes(codes), edges(edges) {}

    void MacroDefined(const clang::Token& MacroNameTok, const clang::MacroDirective* MD) override;

    void InclusionDirective(
        clang::SourceLocation HashLoc,
        const clang::Token &IncludeTok,
        clang::StringRef FileName,
        bool IsAngled,
        clang::CharSourceRange FilenameRange,
        clang::OptionalFileEntryRef File,
        clang::StringRef SearchPath,
        clang::StringRef RelativePath,
        const clang::Module *Imported,
        bool ModuleImported,
        clang::SrcMgr::CharacteristicKind FileType
    ) override;

private:
    std::optional<std::string> getSourceCodeFromSourceLocation(clang::SourceLocation) const;

    bool isRealFile(clang::FileID) const;
};