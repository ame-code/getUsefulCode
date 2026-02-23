#include <clang/Lex/Token.h>
#include <clang/Lex/Lexer.h>

#include "RecordPPCallbacks.h"

using Self = RecordPPCallbacks;

void Self::MacroDefined(const clang::Token& MacroNameTok, const clang::MacroDirective* MD) {
    auto Loc = MacroNameTok.getLocation();
    if (Loc.isInvalid() || SM.isInSystemHeader(Loc)) return;

    clang::FileID FID = SM.getFileID(Loc);
    auto OptionalFileEntryRef = SM.getFileEntryRefForID(FID);
    if (!OptionalFileEntryRef) return;
    auto FileEntryRef = *OptionalFileEntryRef;

    if (auto Code = getSourceCodeFromSourceLocation(Loc)) {
        codes.emplace_back(FileEntryRef.getUniqueID(), SM.getFileOffset(Loc), *Code);
    }
}

void Self::InclusionDirective(
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
) {
    if (HashLoc.isInvalid()) return;

    if (!File) return;
    auto InclusionFileEntry = *File;

    auto OptionalOriginalFileEntry = SM.getFileEntryRefForID(SM.getFileID(HashLoc));
    if (!OptionalOriginalFileEntry) return;
    auto OriginalFileEntry = *OptionalOriginalFileEntry;

    if (visited_file.contains(InclusionFileEntry.getUniqueID())) return;
    visited_file.insert(InclusionFileEntry.getUniqueID());

    if (!SM.isInSystemHeader(HashLoc) && FileType == clang::SrcMgr::C_User) {
        edges.emplace_back(OriginalFileEntry.getUniqueID(), InclusionFileEntry.getUniqueID(), SM.getFileOffset(HashLoc));
    }

    if (FileType != clang::SrcMgr::C_System && FileType != clang::SrcMgr::C_ExternCSystem) return;
    if (SM.isInSystemHeader(HashLoc)) return;

    if (auto Code = getSourceCodeFromSourceLocation(HashLoc)) {
        codes.emplace_back(OriginalFileEntry.getUniqueID(), SM.getFileOffset(HashLoc), *Code);
    }
}

std::optional<std::string> Self::getSourceCodeFromSourceLocation(clang::SourceLocation Start) {
    if (Start.isInvalid() || !Start.isFileID()) return std::nullopt;

    unsigned LineNo = SM.getSpellingLineNumber(Start);
    clang::FileID FID = SM.getFileID(Start);
    std::string Result;

    while (true) {
        clang::SourceLocation lineStart = SM.translateLineCol(FID, LineNo, 1);
        if (lineStart.isInvalid()) return std::nullopt;

        clang::SourceLocation lineEnd = SM.translateLineCol(FID, LineNo + 1, 1);
        if (auto FIleEndLoc = SM.getLocForEndOfFile(FID); FIleEndLoc.getRawEncoding() - lineEnd.getRawEncoding() == 1) {
            lineEnd = FIleEndLoc;
        }

        auto LineText = clang::Lexer::getSourceText(
            clang::CharSourceRange::getCharRange(lineStart, lineEnd),
            SM, LangOpts
        );

        Result += LineText.str();

        if (LineText.trim().ends_with('\\')) {
            LineNo++;
        } else {
            break;
        }
    }

    for (std::set<char> st{' ', '\n', '\t', '\f', '\r'};!Result.empty() && st.contains(Result.back());) {
        Result.pop_back();
    }
    Result.push_back('\n');
    return Result;
}

bool Self::isRealFile(clang::FileID FID) {
    return SM.getFileEntryForID(FID) != nullptr;
}