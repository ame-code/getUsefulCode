#include "MyClangTools.h"
#include "BuildGraphASTVisitor.h"

#include <ranges>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/RecursiveASTVisitor.h>

using Self = BuildGraphASTVisitor;
using Base = clang::RecursiveASTVisitor<Self>;

Self::BuildGraphASTVisitor(clang::ASTContext* context) : context(context), decl_list() {
}

bool Self::shouldVisitTemplateInstantiations() const { // NOLINT(*-convert-member-functions-to-static)
    return true;
}
bool Self::shouldVisitImplicitCode() const { // NOLINT(*-convert-member-functions-to-static)
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
    const bool result = Base::TraverseDecl(D);
    decl_list.pop_back();

    return result;
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

clang::QualType Self::getRemoveRefPtrArrType(clang::QualType QT) {
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
    if (auto* MTE = clang::dyn_cast<clang::MaterializeTemporaryExpr>(S)) {
        handleMaterializeTemporaryExpr(MTE);
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

void Self::handleMaterializeTemporaryExpr(clang::MaterializeTemporaryExpr *MTE) {
    clang::QualType QT = MTE->getType();
    QT = getRemoveRefPtrArrType(QT);
    auto* D = getDeclFromQualType(QT);
    if (!D || !getLastDecl()) return;

    graph[getLastDecl()->getCanonicalDecl()].insert(D->getCanonicalDecl());
}

bool Self::VisitCallExpr(clang::CallExpr* Call) {
    const clang::FunctionDecl* Callee = Call->getDirectCallee();
    if (!Callee || !isInStdNamespace(Callee)) return true;

    auto name = Callee->getName();

    if (name != "format" && name != "print" && name != "println") return true;

    if (!formatter_main_template) return true;
    log("formatter check: {}", formatter_main_template->getQualifiedNameAsString());

    const clang::TemplateArgumentList* args = Callee->getTemplateSpecializationArgs();
    if (!args) return true;

    for (unsigned i = 0; i < args->size(); i++) {
        const auto& arg = args->get(i);
        if (arg.getKind() == clang::TemplateArgument::Pack) {
            for (const auto& pack_arg : arg.pack_elements()) {
                clang::QualType QT = pack_arg.getAsType().getNonReferenceType()
                                                         .getUnqualifiedType()
                                                         .getCanonicalType();

                clang::TemplateArgument formatter_arg(QT), char_t(context->CharTy);

                std::array arr = {formatter_arg, char_t};

                void* InsertPos = nullptr;
                auto* Spec = formatter_main_template->findSpecialization(arr, InsertPos);
                if (!Spec) {
                    for (auto* S : formatter_main_template->specializations()) {
                        const auto& Args = S->getTemplateArgs();
                        if (Args.size() > 0 && Args[0].getKind() == clang::TemplateArgument::Type) {
                            clang::ASTContext::hasSameType(Args[0].getAsType(), QT);
                            Spec = S;
                            break;
                        }
                    }
                }

                if (!Spec) return true;

                clang::NamedDecl* TargetDecl = Spec;
                auto Specialization = Spec->getSpecializedTemplateOrPartial();
                if (auto* Partial = Specialization.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                    TargetDecl = Partial;
                }

                log("line:{}", __LINE__);
                graph[getLastDecl()->getCanonicalDecl()].insert(TargetDecl->getCanonicalDecl());
            }
        }
    }

    return true;
}

bool Self::isInStdNamespace(const clang::Decl* D) {
    const clang::DeclContext* ctx = D->getDeclContext();
    const auto* parent = ctx->getParent();
    return ctx->isStdNamespace() && parent && parent->isTranslationUnit();
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

clang::Decl* Self::getDeclFromQualType(clang::QualType QT) {
    QT = getRemoveRefPtrArrType(QT);
    if (auto* tag_decl = QT->getAsTagDecl())
        return tag_decl;
    if (const clang::TypedefType* using_type = QT->getAs<clang::TypedefType>()) {
        auto decl = using_type->getDecl();
        return decl;
    }
    if (const auto* Spec = QT->getAs<clang::TemplateSpecializationType>()) {
        const auto template_decl = Spec->getTemplateName().getAsTemplateDecl();
        return template_decl;

    }
    return nullptr;
}

void Self::handleVarDecl(clang::VarDecl *VD) {
    const auto CanonicalDecl = VD->getCanonicalDecl();

    clang::QualType QT = VD->getType();

    QT = getRemoveRefPtrArrType(QT);

    auto* T = QT.getTypePtr();

    if (auto* TST = clang::dyn_cast<clang::TemplateSpecializationType>(T)) {
        for (const auto& arg : TST->template_arguments()) {
            if (arg.getKind() == clang::TemplateArgument::Type) {
                const auto decl = getDeclFromQualType(arg.getAsType());

                if (!decl) continue;

                graph[CanonicalDecl].insert(decl->getCanonicalDecl());
            } else if (arg.getKind() == clang::TemplateArgument::Declaration) {

            } else if (arg.getKind() == clang::TemplateArgument::Template) {

            }
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
    auto CanonicalDecl = TATD->getCanonicalDecl();
    const auto* D = TATD->getTemplatedDecl();

    if (auto* DNT = clang::dyn_cast<clang::DependentNameType>(D->getUnderlyingType().getTypePtr())) {
        // llvm::errs() << "Found DependentNameType\n";

        // if (auto* II = DNT->getIdentifier()) {
        //     llvm::errs() << "member name: "
        //                  << II->getName() << "\n";
        // }

        auto qualifier = DNT->getQualifier();
        auto* NNS = &qualifier;

        if (const auto* T = NNS->getAsType()) {
            if (const auto* TST = T->getAs<clang::TemplateSpecializationType>()) {
                clang::TemplateName TN = TST->getTemplateName();
                clang::TemplateDecl* TD = TN.getAsTemplateDecl();
                if (TD) {
                    // llvm::errs() << "Found TemplateDecl " << TD->getNameAsString() << '\n';

                    graph[CanonicalDecl].insert(TD->getCanonicalDecl());
                }
            }
        }
    }
}
