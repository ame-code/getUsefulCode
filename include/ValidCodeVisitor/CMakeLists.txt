cmake_minimum_required(VERSION 3.20)

# lib ValidCodeVisitor
add_library(ValidCodeVisitor STATIC ValidCodeVisitor.h ValidCodeVisitor.cpp)
target_include_directories(ValidCodeVisitor PRIVATE
        ${CLANG_INCLUDE_DIRS}
        ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(ValidCodeVisitor PRIVATE
        clang-cpp
        LLVM
)
