#pragma once

#include <clang/AST/ASTConsumer.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#include "BuildGraphASTVisitor.h"
#include "GetUsefulCodeASTVisitor.h"
#include "RecordPPCallbacks.h"
#include "MyClangTools.h"

class GetUsefulCodeASTConsumer : public clang::ASTConsumer {
    BuildGraphASTVisitor graph_visitor;
    GetUsefulCodeASTVisitor valid_code_visitor;
    std::vector<CodeInfo>& codes;
    std::vector<Edge>& edges;
    std::string& valid_code;

public:
    GetUsefulCodeASTConsumer(clang::ASTContext* context, std::vector<CodeInfo>& codes, std::vector<Edge>& edges, std::string& valid_code): graph_visitor(context), valid_code_visitor(context), codes(codes), edges(edges), valid_code(valid_code) {}

    void HandleTranslationUnit(clang::ASTContext&) override;
    
private:
    clang::ClassTemplateDecl* findFormatterTemplate(clang::ASTContext& context);

    static std::unordered_set<clang::Decl*> markValidDecl(const std::unordered_map<clang::Decl*, std::unordered_set<clang::Decl*>>&, clang::FunctionDecl*);
};