#include "MyClangTools.h"

#include <format>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Redeclarable.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/Casting.h>

clang::ClassTemplateDecl* getPrimaryTemplateFromClass(clang::Decl* D) {
    if (auto* CTD = clang::dyn_cast<clang::ClassTemplateDecl>(D))
        return CTD;
    
    if (auto* Spec = 
        clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(D))
        return Spec->getSpecializedTemplate();

    return nullptr;
}

clang::FunctionTemplateDecl* getPrimaryTemplateFromFunction(clang::Decl* D) {
    if (clang::FunctionDecl* FD = clang::dyn_cast<clang::FunctionDecl>(D)) {
        if (
            FD->isTemplateInstantiation() || FD->isFunctionTemplateSpecialization()
        ) {
            if (clang::FunctionDecl* Pattern = FD->getTemplateInstantiationPattern()) {
                if (clang::FunctionTemplateDecl* FTD = Pattern->getDescribedFunctionTemplate()) {
                    return FTD;
                }
            }
        }
    }

    return nullptr;
}

clang::Decl* getPrimaryTemplate(clang::Decl* D) {
    if (auto* result = getPrimaryTemplateFromClass(D))
        return result;
    else if (auto* result = getPrimaryTemplateFromFunction(D))
        return result;
    else
        return nullptr;
}

clang::Decl* getReferencedDecl(clang::Stmt* S) {
    if (!S)
        return nullptr;

    if (auto* DRE = clang::dyn_cast<clang::DeclRefExpr>(S))
        return DRE->getDecl();
    else if (auto* ME = clang::dyn_cast<clang::MemberExpr>(S))
        return ME->getMemberDecl();
    else if (auto* CE = clang::dyn_cast<clang::CallExpr>(S))
        return CE->getDirectCallee();
    else if (auto* CCE = clang::dyn_cast<clang::CXXConstructExpr>(S))
        return CCE->getConstructor();
    return nullptr;
}

void logDeclPos(clang::Decl* D) {
    auto& ctx = D->getASTContext();
    const auto& SM = ctx.getSourceManager();
    log("file:{}, line:{}, column:{}", SM.getFilename(D->getLocation()).str(), SM.getLineNumber(D->getLocation()), SM.getColumnNumber(D->getLocation()));
}