#include "MyClangTools.h"
#include "BuildGraphASTVisitor.h"

#include <clang/Basic/SourceManager.h>
#include <clang/AST/RecursiveASTVisitor.h>

using Self = BuildGraphASTVisitor;
using Base = clang::RecursiveASTVisitor<Self>;

Self::BuildGraphASTVisitor(clang::ASTContext* context) : context(context), decl_list() {
}

bool Self::shouldVisitTemplateInstantiations() { // NOLINT(*-convert-member-functions-to-static)
    return true;
}
bool Self::shouldVisitImplicitCode() { // NOLINT(*-convert-member-functions-to-static)
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
        } else if (clang::isa<clang::BuiltinType>(T)) {
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

void Self::handleMaterializeTemporaryExpr(const clang::MaterializeTemporaryExpr *MTE) {
    clang::QualType QT = MTE->getType();
    QT = getRemoveRefPtrArrType(QT);
    auto* D = getDeclFromQualType(QT);
    if (!D || !getLastDecl()) return;

    graph[getLastDecl()->getCanonicalDecl()].insert(D->getCanonicalDecl());
}

bool Self::VisitCallExpr(clang::CallExpr* Call) {
    const clang::FunctionDecl* Callee = Call->getDirectCallee();
    if (!Callee) return true;

    if (!formatter_main_template) return true;

    auto name = Callee->getQualifiedNameAsString();

    if (name != "std::format" && name != "std::print" && name != "std::println") return true;

    const clang::TemplateArgumentList* args = Callee->getTemplateSpecializationArgs();
    if (!args) return true;

    for (unsigned i = 0; i < args->size(); i++) {
        if (args->get(i).getKind() == clang::TemplateArgument::Pack) {
            for (const auto& arg : args->get(i).pack_elements()) {
                clang::QualType QT = arg.getAsType().getNonReferenceType().getUnqualifiedType().getCanonicalType();
                void* InsertPos = nullptr;
                std::array<clang::TemplateArgument, 2> args_array{clang::TemplateArgument(QT), clang::TemplateArgument(context->CharTy)};
                auto* Spec = formatter_main_template->findSpecialization(args_array, InsertPos);
                if (!Spec) {
                    for (auto* S :formatter_main_template->specializations()) {
                        if (const auto& formatter_args = S->getTemplateArgs();
                            formatter_args.size() && formatter_args[0].getKind() == clang::TemplateArgument::Type
                        ) {
                            clang::ASTContext::hasSameType(formatter_args[0].getAsType(), QT);
                            Spec = S;
                            break;
                        }
                    }
                }

                if (!Spec) continue;

                const auto SpecializationPos = Spec->getSpecializedTemplateOrPartial();
                if (auto* Partial = SpecializationPos.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                    auto& SM = context->getSourceManager();
                    if (!context->getSourceManager().isInSystemHeader(Partial->getLocation())) {
                        graph[getLastDecl()->getCanonicalDecl()].insert(Partial->getCanonicalDecl());
                    } else {
                        clang::CXXRecordDecl* formated_type_record_decl = arg.getAsType().getNonReferenceType().getUnqualifiedType().getCanonicalType()->getAsCXXRecordDecl();
                        std::string formated_type_name = formated_type_record_decl ? formated_type_record_decl->getQualifiedNameAsString() : "";
                        const auto& formatter_args = Spec->getTemplateInstantiationArgs();
                        if (formated_type_name == "std::stack" && formatter_args.size() == 4 && formatter_args[2].getKind() == clang::TemplateArgument::Type) {
                            const auto& stack_container_arg = formatter_args[2];
                            clang::QualType stack_container_type = stack_container_arg.getAsType().getNonReferenceType().getUnqualifiedType().getCanonicalType();
                            if (clang::ClassTemplateSpecializationDecl* stack_container_formatter_decl = formatter_main_template->findSpecialization({clang::TemplateArgument(stack_container_type), clang::TemplateArgument(context->CharTy)}, InsertPos)) {
                                const auto spec_from = stack_container_formatter_decl->getSpecializedTemplateOrPartial();
                                if (auto* from_partial = spec_from.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(from_partial->getCanonicalDecl());
                                } else {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(stack_container_formatter_decl->getCanonicalDecl());
                                }
                            }
                        } else if (formated_type_name == "std::queue" && formatter_args.size() == 4 && formatter_args[2].getKind() == clang::TemplateArgument::Type) {
                            const auto& queue_container_arg = formatter_args[2];
                            clang::QualType queue_container_type = queue_container_arg.getAsType().getNonReferenceType().getUnqualifiedType().getCanonicalType();

                            if (clang::ClassTemplateSpecializationDecl* queue_container_formatter_decl = formatter_main_template->findSpecialization({clang::TemplateArgument(queue_container_type), clang::TemplateArgument(context->CharTy)}, InsertPos)) {
                                const auto spec_from = queue_container_formatter_decl->getSpecializedTemplateOrPartial();
                                if (auto* from_partial = spec_from.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(from_partial->getCanonicalDecl());
                                } else {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(queue_container_formatter_decl->getCanonicalDecl());
                                }
                            }
                        } else if (formated_type_name == "std::priority_queue" && formatter_args.size() == 4 && formatter_args[2].getKind() == clang::TemplateArgument::Type) {
                            const auto& priority_queue_container_arg = formatter_args[2];
                            clang::QualType priority_queue_container_type = priority_queue_container_arg.getAsType().getNonReferenceType().getUnqualifiedType().getCanonicalType();

                            if (clang::ClassTemplateSpecializationDecl* priority_queue_container_formatter_decl = formatter_main_template->findSpecialization({clang::TemplateArgument(priority_queue_container_type), clang::TemplateArgument(context->CharTy)}, InsertPos)) {
                                const auto spec_from = priority_queue_container_formatter_decl->getSpecializedTemplateOrPartial();
                                if (auto* from_partial = spec_from.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(from_partial->getCanonicalDecl());
                                } else {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(priority_queue_container_formatter_decl->getCanonicalDecl());
                                }
                            }
                        } else if (formated_type_name == "std::pair" && formatter_args.size() == 2 && formatter_args[1].getKind() == clang::TemplateArgument::Pack) {
                            for (const auto& pair_element_arg : formatter_args[1].pack_elements()) {
                                if (pair_element_arg.getKind() == clang::TemplateArgument::Type) {
                                    clang::QualType pair_element_type = pair_element_arg.getAsType().getNonReferenceType().getUnqualifiedType().getCanonicalType();
                                    if (clang::ClassTemplateSpecializationDecl* pair_element_decl = formatter_main_template->findSpecialization({clang::TemplateArgument(pair_element_type), clang::TemplateArgument(context->CharTy)}, InsertPos)) {
                                        const auto spec_from = pair_element_decl->getSpecializedTemplateOrPartial();
                                        if (auto* from_partial = spec_from.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                                            graph[getLastDecl()->getCanonicalDecl()].insert(from_partial->getCanonicalDecl());
                                        } else {
                                            graph[getLastDecl()->getCanonicalDecl()].insert(pair_element_decl->getCanonicalDecl());
                                        }
                                    }
                                }
                            }
                        } else if (formated_type_name == "std::tuple" && formatter_args.size() == 2 && formatter_args[1].getKind() == clang::TemplateArgument::Pack) {
                            for (const auto& tuple_element_arg : formatter_args.asArray()) {
                                if (tuple_element_arg.getKind() == clang::TemplateArgument::Type) {
                                    clang::QualType tuple_element_type = tuple_element_arg.getAsType().getNonReferenceType().getUnqualifiedType().getCanonicalType();
                                    if (clang::ClassTemplateSpecializationDecl* tuple_element_decl = formatter_main_template->findSpecialization({clang::TemplateArgument(tuple_element_type), clang::TemplateArgument(context->CharTy)}, InsertPos)) {
                                        const auto spec_from = tuple_element_decl->getSpecializedTemplateOrPartial();
                                        if (auto* from_partial = spec_from.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                                            graph[getLastDecl()->getCanonicalDecl()].insert(from_partial->getCanonicalDecl());
                                        } else {
                                            graph[getLastDecl()->getCanonicalDecl()].insert(tuple_element_decl->getCanonicalDecl());
                                        }
                                    }
                                }
                            }
                        } else if (clang::QualType range_element_type = getRangeElementType(QT); !range_element_type.isNull()) {
                            range_element_type = range_element_type.getNonReferenceType().getUnqualifiedType().getCanonicalType();
                            if (auto* range_element_formatter_decl = formatter_main_template->findSpecialization({clang::TemplateArgument(range_element_type), clang::TemplateArgument(context->CharTy)}, InsertPos)) {
                                const auto spec_from = range_element_formatter_decl->getSpecializedTemplateOrPartial();
                                if (auto* from_partial = spec_from.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(from_partial->getCanonicalDecl());
                                } else {
                                    graph[getLastDecl()->getCanonicalDecl()].insert(range_element_formatter_decl->getCanonicalDecl());
                                }
                            }
                        }
                    }
                } else {
                    if (!context->getSourceManager().isInSystemHeader(Spec->getLocation())) {
                        graph[getLastDecl()->getCanonicalDecl()].insert(Spec->getCanonicalDecl());
                    }
                }
            }
        }
    }

    return true;
}

bool Self::VisitCastExpr(clang::CastExpr* CE) {
    auto* target_type_decl = getDeclFromQualType(CE->getType());

    if (!target_type_decl) return true;

    graph[getLastDecl()->getCanonicalDecl()].insert(target_type_decl->getCanonicalDecl());

    return true;
}


clang::ClassTemplateSpecializationDecl* Self::findSpecFromFormatter(clang::QualType QT) const {
    QT = QT.getNonReferenceType().getUnqualifiedType().getCanonicalType();
    void* InsertPos = nullptr;
    std::array args_array{clang::TemplateArgument(QT), clang::TemplateArgument(context->CharTy)};
    auto* Spec = formatter_main_template->findSpecialization(args_array, InsertPos);
    if (!Spec) {
        for (auto* S : formatter_main_template->specializations()) {
            if (const auto& Args = S->getTemplateArgs(); Args.size() > 0 && Args[0].getKind() == clang::TemplateArgument::Type) {
                clang::ASTContext::hasSameType(Args[0].getAsType(), QT);
                Spec = S;
                break;
            }
        }
    }
    return Spec;
}

clang::NamedDecl *Self::getFormatterDeclFromQualType(const clang::QualType QT) const {
    auto* Spec = findSpecFromFormatter(QT);

    if (!Spec) return nullptr;

    clang::NamedDecl* TargetDecl = Spec;
    const auto Specialization = Spec->getSpecializedTemplateOrPartial();
    if (auto* Partial = Specialization.dyn_cast<clang::ClassTemplatePartialSpecializationDecl*>()) {
        TargetDecl = Partial;
    }

    return TargetDecl;
}

clang::QualType Self::getRangeElementType(clang::QualType R) const {
    R = R.getNonReferenceType().getUnqualifiedType().getCanonicalType();
    if (R->isArrayType()) {
        return R->getAsArrayTypeUnsafe()->getElementType();
    }

    clang::QualType IterType;
    const auto* RD = R->getAsCXXRecordDecl();
    if (RD) {
        for (auto BeginLookup = RD->lookup(&context->Idents.get("begin")); auto* D : BeginLookup) {
            if (auto* MD = clang::dyn_cast<clang::CXXMethodDecl>(D)) {
                IterType = MD->getType();
                break;
            }
        }
    }

    clang::QualType ElementType;
    if (!IterType.isNull()) {
        if (IterType->isPointerType()) {
            ElementType = IterType->getPointeeType();
        } else {
            clang::DeclarationName OpName = context->DeclarationNames.getCXXOperatorName(clang::OO_Star);
            if (RD) {
                for (auto LookupRes = RD->lookup(OpName); auto* D : LookupRes) {
                    if (const auto* MD = clang::dyn_cast<clang::CXXMethodDecl>(D)) {
                        ElementType = MD->getReturnType();
                        break;
                    }
                }
            }
        }
    }
    return ElementType;
}

bool Self::isInStdNamespace(const clang::Decl* D) {
    const clang::DeclContext* ctx = D->getDeclContext();
    const auto* parent = ctx->getParent();
    return ctx->isStdNamespace() && parent && parent->isTranslationUnit();
}

clang::Decl* Self::getLastDecl() const {
    return decl_list.empty() ? nullptr : decl_list.back();
}

clang::Decl* Self::getParentDecl(const clang::Decl* D) const {
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
    } else if (clang::isa<clang::TypedefType>(T)) {
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
    if (const clang::TypedefType* using_type = QT->getAs<clang::TypedefType>()) {
        auto decl = using_type->getDecl();
        return decl;
    }
    if (const auto* Spec = QT->getAs<clang::TemplateSpecializationType>()) {
        const auto template_decl = Spec->getTemplateName().getAsTemplateDecl();
        return template_decl;
    }
    if (auto* tag_decl = QT->getAsTagDecl()) {
        return tag_decl;
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
    } else if (clang::isa<clang::TypedefType>(T)) {
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
                const clang::TemplateName TN = TST->getTemplateName();
                if (clang::TemplateDecl* TD = TN.getAsTemplateDecl()) {
                    // llvm::errs() << "Found TemplateDecl " << TD->getNameAsString() << '\n';

                    graph[CanonicalDecl].insert(TD->getCanonicalDecl());
                }
            }
        }
    }
}
