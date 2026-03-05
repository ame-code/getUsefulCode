#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>


#include <unordered_map>
#include <unordered_set>
#include <vector>

class BuildGraphASTVisitor : public clang::RecursiveASTVisitor<BuildGraphASTVisitor> {
public:
    BuildGraphASTVisitor(clang::ASTContext* context);

    bool TraverseDecl(clang::Decl*);
    bool TraverseStmt(clang::Stmt*);
    bool shouldVisitTemplateInstantiations() const;
    bool shouldVisitImplicitCode() const;
    bool VisitDecl(clang::Decl*);
    bool VisitStmt(clang::Stmt*);

    auto& getGraph() & {
        return graph;
    }

    const auto& getGraph() const & {
        return graph;
    }

    auto&& getGraph() && {
        return std::move(graph);
    }

    const auto&& getGraph() const && {
        return std::move(graph);
    }
private:
    clang::Decl* getLastDecl() const;

    clang::Decl* getParentDecl(clang::Decl* D) const;

    clang::Decl* getRemoveRefPtrArrTypeDecl(clang::QualType);

    void handleNonTypeTemplateParmDecl(clang::NonTypeTemplateParmDecl* NTTP);
    void handleVarDecl(clang::VarDecl* VD);
    void handleTypedefNameDecl(clang::TypedefNameDecl* TND);
    void handleTypeAliasTemplateDecl(clang::TypeAliasTemplateDecl* TATD);

    clang::ASTContext* context;
    std::vector<clang::Decl*> decl_list;
    std::unordered_map<clang::Decl*, std::unordered_set<clang::Decl*>> graph;
};