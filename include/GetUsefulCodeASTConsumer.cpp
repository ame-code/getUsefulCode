#include <llvm/Support/FileSystem/UniqueID.h>

#include "GetUsefulCodeASTConsumer.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>

using Self = GetUsefulCodeASTConsumer;
using Base = clang::ASTConsumer;

void Self::HandleTranslationUnit(clang::ASTContext& Context) {
    const auto TUDecl = Context.getTranslationUnitDecl();

    graph_visitor.buildFormatterCacheInTranslationUnit();
    graph_visitor.TraverseDecl(TUDecl);

    const auto DependenceGraph = std::move(graph_visitor).getGraph();

    clang::FunctionDecl* MainFunctionDecl{};
    for (auto* D : TUDecl->decls()) {
        if (auto* FD = clang::dyn_cast<clang::FunctionDecl>(D)) {
            if (FD->getNameInfo().getName().getAsString() == "main") {
                MainFunctionDecl = FD->getCanonicalDecl();
            }
        }
    }

    auto ValidDecls = markValidDecl(DependenceGraph, MainFunctionDecl);

    valid_code_visitor.setValidDecl(std::move(ValidDecls));
    for (auto* D : TUDecl->decls()) {
        valid_code_visitor.VisitDecl(D);
    }

    auto ValidCodes = std::move(valid_code_visitor).getValidCodes();
    std::ranges::move(ValidCodes, std::back_inserter(codes));

    std::map<llvm::sys::fs::UniqueID, std::vector<CodeInfo::Position>> order;
    std::map<llvm::sys::fs::UniqueID, std::set<std::pair<llvm::sys::fs::UniqueID, unsigned>>> InclusionGraph;

    for (auto [u, v, w] : edges) {
        InclusionGraph[u].insert({v, w});
    }

    auto& SM = Context.getSourceManager();
    auto MainFileID = SM.getMainFileID();
    auto OptionalMainFileEntryRef = SM.getFileEntryRefForID(MainFileID);
    if (!OptionalMainFileEntryRef) return;
    auto MainFileEntryRef = *OptionalMainFileEntryRef;
    auto MainFileUniqueID = MainFileEntryRef.getUniqueID();
    
    auto buildOrder = [&](auto&& self, llvm::sys::fs::UniqueID u) -> void {
        for (auto [v, w] : InclusionGraph[u]) {
            std::ranges::copy(order[u], std::back_inserter(order[v]));
            order[v].push_back({u, w});
            self(self, v);
        }
    };
    buildOrder(buildOrder, MainFileUniqueID);

    auto compareCodeInfo = [&](const CodeInfo& lhs, const CodeInfo& rhs) -> bool {
        if (lhs.pos.id != rhs.pos.id) {
            auto& lhsOrder = order[lhs.pos.id];
            auto& rhsOrder = order[rhs.pos.id];
            lhsOrder.push_back(lhs.pos);
            rhsOrder.push_back(rhs.pos);
            const auto n = std::ranges::min(std::ssize(lhsOrder), std::ssize(rhsOrder));
            for (int i = 0; i < n; i++) {
                if (lhsOrder[i].offset != rhsOrder[i].offset) {
                    const auto result = lhsOrder[i].offset < rhsOrder[i].offset;
                    lhsOrder.pop_back();
                    rhsOrder.pop_back();
                    return result;
                }
            }
            lhsOrder.pop_back();
            rhsOrder.pop_back();
            return false;
        } else {
            return lhs.pos.offset < rhs.pos.offset;
        }
    };

    std::ranges::sort(codes, compareCodeInfo);

    for (const auto& code_info : codes) {
        valid_code.append(code_info.code);
    }
}

std::unordered_set<clang::Decl*> Self::markValidDecl(const std::unordered_map<clang::Decl*, std::unordered_set<clang::Decl*>>& DependenceGraph, clang::FunctionDecl* MainFunctionDecl) {
    std::unordered_set<clang::Decl*> ValidDecls, vis;
    auto dfs = [&](auto&& self, clang::Decl* u) -> void {
        if (vis.contains(u)) return;
        vis.insert(u);

        ValidDecls.insert(u);
        const auto it = DependenceGraph.find(u);
        if (it == DependenceGraph.end()) return;
        for (auto v : it->second) {
            self(self, v);
        }
    };
    dfs(dfs, MainFunctionDecl->getCanonicalDecl());

    return ValidDecls;
}