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

    static bool shouldVisitTemplateInstantiations();

    static bool shouldVisitImplicitCode();
    bool VisitDecl(clang::Decl*);
    bool VisitStmt(clang::Stmt*);
    bool VisitCallExpr(clang::CallExpr*);

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

    clang::ClassTemplateDecl* formatter_main_template = nullptr;

private:
    clang::Decl* getLastDecl() const;

    clang::Decl* getParentDecl(const clang::Decl* D) const;

    static clang::Decl* getRemoveRefPtrArrTypeDecl(clang::QualType);
    static clang::QualType getRemoveRefPtrArrType(clang::QualType QT);

    clang::ClassTemplateSpecializationDecl* findSpecFromFormatter(clang::QualType QT) const;

    clang::NamedDecl* getFormatterDeclFromQualType(clang::QualType QT) const;

    clang::QualType getRangeElementType(clang::QualType R) const;

    static bool isInStdNamespace(const clang::Decl* D);

    static clang::Decl* getDeclFromQualType(clang::QualType QT);

    void handleNonTypeTemplateParmDecl(clang::NonTypeTemplateParmDecl* NTTP);
    void handleVarDecl(clang::VarDecl* VD);
    void handleTypedefNameDecl(clang::TypedefNameDecl* TND);
    void handleTypeAliasTemplateDecl(clang::TypeAliasTemplateDecl* TATD);
    void handleMaterializeTemporaryExpr(const clang::MaterializeTemporaryExpr* MTE);

    clang::ASTContext* context;
    std::vector<clang::Decl*> decl_list;
    std::unordered_map<clang::Decl*, std::unordered_set<clang::Decl*>> graph;
};