#include "MyClangTools.h"
#include "BuildGraphASTVisitor.h"

#include <clang/Basic/SourceManager.h>
#include <clang/AST/RecursiveASTVisitor.h>

using Self = BuildGraphASTVisitor;
using Base = clang::RecursiveASTVisitor<Self>;

Self::BuildGraphASTVisitor(clang::ASTContext* context): context(context), decl_list() {}


bool Self::shouldVisitTemplateInstantiations() const {
    return true;
}
bool Self::shouldVisitImplicitCode() const {
    return true;
}

bool Self::TraverseDecl(clang::Decl* D) {
    if (!D) {
        return true;
    }
    auto& SM = context->getSourceManager();
    if (SM.isInSystemHeader(D->getLocation())) {
        return true;
    }

    decl_list.push_back(D);
    Base::TraverseDecl(D);
    decl_list.pop_back();

    return true;
}

bool Self::TraverseStmt(clang::Stmt* S) {
    if (!S) {
        return true;
    }
    auto& SM = context->getSourceManager();
    if (SM.isInSystemHeader(S->getBeginLoc()) || SM.isInSystemHeader(S->getEndLoc())) {
        return true;
    }

    return Base::TraverseStmt(S);
}

clang::Decl* Self::getRemoveRefPtrArrTypeDecl(clang::QualType QT) {
    while (true) {
        QT = QT.getLocalUnqualifiedType();

        auto* T = QT.getTypePtr();
        if (auto* ArrT = clang::dyn_cast<clang::ArrayType>(T)) {
            QT = ArrT->getElementType();
        } else if (auto* RefT = clang::dyn_cast<clang::ReferenceType>(T)) {
            QT = RefT->getPointeeType();
        } else if (auto* PtrT = clang::dyn_cast<clang::PointerType>(T)) {
            QT = PtrT->getPointeeType();
        } else if (auto* TDT = clang::dyn_cast<clang::TypedefType>(T)) {
            return TDT->getDecl();
        } else if (auto* RecordT = clang::dyn_cast<clang::RecordType>(T)) {
            return RecordT->getDecl();
        } else if (auto* EnumT = clang::dyn_cast<clang::EnumType>(T)) {
            return EnumT->getDecl();
        } else if (auto* TST = clang::dyn_cast<clang::TemplateSpecializationType>(T)) {
            return TST->getTemplateName().getAsTemplateDecl();
        } else if (auto* BT = clang::dyn_cast<clang::BuiltinType>(T)) {
            return nullptr;
        } else {
            return nullptr;
        }
    }
}

clang::QualType getRemoveRefPtrArrType(clang::QualType QT) {
    while (true) {
        QT = QT.getLocalUnqualifiedType();

        auto* T = QT.getTypePtr();
        if (auto* ArrT = clang::dyn_cast<clang::ArrayType>(T)) {
            QT = ArrT->getElementType();
        } else if (auto* RefT = clang::dyn_cast<clang::ReferenceType>(T)) {
            QT = RefT->getPointeeType();
        } else if (auto* PtrT = clang::dyn_cast<clang::PointerType>(T)) {
            QT = PtrT->getPointeeType();
        } else if (auto member_type = clang::dyn_cast<clang::MemberPointerType>(T)) {
            QT = member_type->getPointeeType();
        } else {
            return QT;
        }
    }
}

bool Self::VisitDecl(clang::Decl* D) {
    if (D->getBeginLoc().isValid()) {
        llvm::errs() << std::format("Decl kind:{}, loc: {},{}\n",
            D->getDeclKindName(),
            context->getSourceManager().getLineNumber(D->getBeginLoc()),
            context->getSourceManager().getColumnNumber(D->getBeginLoc())
        );
    } else {
        llvm::errs() << std::format("Decl kind:{}, loc: error\n",
            D->getDeclKindName()
        );
    }

    auto* CanonicalDecl = D->getCanonicalDecl();

    if (auto* NTTP = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(D)) {
        handleNonTypeTemplateParmDecl(NTTP);
    } else if (auto* VD = clang::dyn_cast<clang::VarDecl>(D)) {
        handleVarDecl(VD);
    } else if (auto* TND = clang::dyn_cast<clang::TypedefNameDecl>(D)) {
        handleTypedefNameDecl(TND);
    } else if (auto* TATD = clang::dyn_cast<clang::TypeAliasTemplateDecl>(D)) {
        handleTypeAliasTemplateDecl(TATD);
    } else if (auto* PrimaryTemplateDecl = getPrimaryTemplate(D)) {
        graph[CanonicalDecl].insert(PrimaryTemplateDecl->getCanonicalDecl());
        graph[PrimaryTemplateDecl->getCanonicalDecl()].insert(CanonicalDecl);
    }

    if (auto* ParentDecl = getParentDecl(D)) {
        auto* CPD = ParentDecl->getCanonicalDecl();

        graph[CanonicalDecl].insert(CPD);

        if(!clang::isa<clang::NamespaceBaseDecl, clang::TranslationUnitDecl>(CPD)) {
            graph[CPD].insert(CanonicalDecl);
        }
    }

    return true;
}

bool Self::VisitStmt(clang::Stmt* S) {
    if (S->getBeginLoc().isValid()) {
        llvm::errs() << std::format("Stmt kind:{}, loc: {},{}\n",
            S->getStmtClassName(),
            context->getSourceManager().getLineNumber(S->getBeginLoc()),
            context->getSourceManager().getColumnNumber(S->getBeginLoc())
        );
    } else {
        llvm::errs() << std::format("Stmt kind:{}, loc: error\n",
            S->getStmtClassName()
        );
    }

    auto* RefedDecl = getReferencedDecl(S);
    if (!RefedDecl) return true;
    
    auto* LastDecl = getLastDecl();
    if (!LastDecl) return true;

    graph[LastDecl->getCanonicalDecl()].insert(RefedDecl->getCanonicalDecl());

    if (auto* PrimaryTemplateDecl = getPrimaryTemplate(RefedDecl)) {
        graph[RefedDecl->getCanonicalDecl()].insert(PrimaryTemplateDecl->getCanonicalDecl());
    }

    return true;
}

clang::Decl* Self::getLastDecl() const {
    return decl_list.empty() ? nullptr : decl_list.back();
}

clang::Decl* Self::getParentDecl(clang::Decl* D) const {
    clang::Decl* result = nullptr;
    for (auto i = std::ssize(decl_list) - 1; i >= 0; i--) {
        if (decl_list[i] == D && i != 0) {
            result = decl_list[i - 1];
        }
    }
    return result;
}

void Self::handleNonTypeTemplateParmDecl(clang::NonTypeTemplateParmDecl* NTTP) {
    const auto CanonicalDecl = NTTP->getCanonicalDecl();

    clang::QualType QT = NTTP->getType();

    QT = getRemoveRefPtrArrType(QT);

    auto* T = QT.getTypePtr();

    if (auto* TST = clang::dyn_cast<clang::TemplateSpecializationType>(T)) {
        for (const auto& arg : TST->template_arguments()) {
            if (arg.getKind() != clang::TemplateArgument::Type) continue;
            const auto type = getRemoveRefPtrArrType(arg.getAsType());
            const auto decl = type->getAsTagDecl();
            if (!decl) continue;
            graph[CanonicalDecl].insert(decl->getCanonicalDecl());
        }

        const auto TN = TST->getTemplateName();
        if (auto* TD = TN.getAsTemplateDecl()) {
            graph[CanonicalDecl].insert(TD->getCanonicalDecl());
        }
    } else if (auto* TDT = clang::dyn_cast<clang::TypedefType>(T)) {
        if (auto* TND = getRemoveRefPtrArrTypeDecl(QT)) {
            graph[CanonicalDecl].insert(TND->getCanonicalDecl());
        }
    } else if (auto* RT = clang::dyn_cast<clang::RecordType>(T)) {
        if (auto* RD = RT->getDecl()) {
            graph[CanonicalDecl].insert(RD->getCanonicalDecl());
        }
    }
}

void Self::handleVarDecl(clang::VarDecl *VD) {
    const auto CanonicalDecl = VD->getCanonicalDecl();

    clang::QualType QT = VD->getType();

    QT = getRemoveRefPtrArrType(QT);

    auto* T = QT.getTypePtr();

    if (auto* TST = clang::dyn_cast<clang::TemplateSpecializationType>(T)) {
        for (const auto& arg : TST->template_arguments()) {
            if (arg.getKind() != clang::TemplateArgument::Type) continue;
            const auto type = getRemoveRefPtrArrType(arg.getAsType());
            const auto decl = type->getAsTagDecl();
            if (!decl) continue;
            graph[CanonicalDecl].insert(decl->getCanonicalDecl());
        }

        const auto TN = TST->getTemplateName();
        if (auto* TD = TN.getAsTemplateDecl()) {
            graph[CanonicalDecl].insert(TD->getCanonicalDecl());
        }
    } else if (auto* TDT = clang::dyn_cast<clang::TypedefType>(T)) {
        if (auto* TND = getRemoveRefPtrArrTypeDecl(QT)) {
            graph[CanonicalDecl].insert(TND->getCanonicalDecl());
        }
    } else if (auto* RT = clang::dyn_cast<clang::RecordType>(T)) {
        if (auto* RD = RT->getDecl()) {
            graph[CanonicalDecl].insert(RD->getCanonicalDecl());
        }
    }
}

void Self::handleTypedefNameDecl(clang::TypedefNameDecl *TND) {
    const auto CanonicalDecl = TND->getCanonicalDecl();

    const auto QT = TND->getUnderlyingType();

    if (auto* UnderlyingDecl = getRemoveRefPtrArrTypeDecl(QT)) {
        graph[CanonicalDecl].insert(UnderlyingDecl->getCanonicalDecl());
    }
}

void Self::handleTypeAliasTemplateDecl(clang::TypeAliasTemplateDecl *TATD) {
    auto* D = TATD->getTemplatedDecl();

    clang::QualType QT = D->getUnderlyingType();
    const clang::Type* T = QT.getTypePtr();
    if (auto* DNT = clang::dyn_cast<clang::DependentNameType>(T)) {
        llvm::errs() << "Found DependentNameType\n";

        if (auto* II = DNT->getIdentifier()) {
            llvm::errs() << "member name: "
                         << II->getName() << "\n";
        }

        auto qualifier = DNT->getQualifier();

        const auto* NNS = &qualifier;

        while (NNS) {
            if (auto* T = NNS->getAsType()) {
                auto QT = clang::QualType(T, 0);

                if (auto* TST = QT->getAs<clang::TemplateSpecializationType>()) {
                    clang::TemplateName TN = TST->getTemplateName();
                    if ()
                }
            }
        }

        auto qualifier = DNT->getQualifier();
    }
}
