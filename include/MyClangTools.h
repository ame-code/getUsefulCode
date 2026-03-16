#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>

struct CodeInfo {
    struct Position {
        llvm::sys::fs::UniqueID id;
        unsigned offset;
    };

    CodeInfo(llvm::sys::fs::UniqueID id, unsigned offset, std::string code): pos(id, offset), code(std::move(code)) {}

    Position pos;
    std::string code;
};

struct Edge {
    llvm::sys::fs::UniqueID u, v;
    unsigned w;
};

clang::ClassTemplateDecl* getPrimaryTemplateFromClass(clang::Decl* D);

clang::FunctionTemplateDecl* getPrimaryTemplateFromFunction(clang::Decl* D);

clang::Decl* getPrimaryTemplate(clang::Decl* D);

clang::Decl* getReferencedDecl(clang::Stmt* S);

#define log(...) llvm::errs() << "[log] " << std::format(__VA_ARGS__) << '\n'
#define logLine() log("line:{}", __LINE__)
#define logFileAndLine() log("file:{}, line:{}", __FILE__, __LINE__)

void logDeclPos(clang::Decl* D);