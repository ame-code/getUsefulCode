cmake_minimum_required(VERSION 3.20)

find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)

# Lib MyClangTools
add_library(MyClangTools STATIC
    MyClangTools.h
    MyClangTools.cpp
)
target_include_directories(MyClangTools PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(MyClangTools PRIVATE
    clang-cpp
    LLVM
)

# Lib BuildGraphASTVisitor
add_library(BuildGraphASTVisitor STATIC BuildGraphASTVisitor.h BuildGraphASTVisitor.cpp)
target_include_directories(BuildGraphASTVisitor PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(BuildGraphASTVisitor PRIVATE
    clang-cpp
    LLVM
    MyClangTools
)

# Lib RecordPPCallbacks
add_library(RecordPPCallbacks STATIC RecordPPCallbacks.h RecordPPCallbacks.cpp)
target_include_directories(RecordPPCallbacks PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(RecordPPCallbacks PRIVATE
    clang-cpp
    LLVM
    MyClangTools
)

# Lib GetValidCodeASTVisitor
add_library(GetValidCodeASTVisitor STATIC GetUsefulCodeASTVisitor.h GetUsefulCodeASTVisitor.cpp)
target_include_directories(GetValidCodeASTVisitor PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(GetValidCodeASTVisitor PRIVATE
    clang-cpp
    LLVM
    MyClangTools
)

# Lib GetValidCodeASTConsumer
add_library(GetValidCodeASTConsumer STATIC GetUsefulCodeASTConsumer.h GetUsefulCodeASTConsumer.cpp)
target_include_directories(GetValidCodeASTConsumer PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(GetValidCodeASTConsumer PRIVATE
    clang-cpp
    LLVM
    MyClangTools
    BuildGraphASTVisitor
    GetValidCodeASTVisitor
)

# Lib GetValidCodeASTAction
add_library(GetValidCodeASTAction STATIC GetUsefulCodeASTAction.h GetUsefulCodeASTAction.cpp)
target_include_directories(GetValidCodeASTAction PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(GetValidCodeASTAction PRIVATE
    clang-cpp
    LLVM
    MyClangTools
    RecordPPCallbacks
    GetValidCodeASTConsumer
)

# Lib GetValidCodeFrontendActionFactory
add_library(GetValidCodeFrontendActionFactory STATIC GetUsefulCodeFrontendActionFactory.h GetUsefulCodeFrontendActionFactory.cpp)
target_include_directories(GetValidCodeFrontendActionFactory PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(GetValidCodeFrontendActionFactory PRIVATE
    clang-cpp
    LLVM
    GetValidCodeASTAction
)

# Lib MyCompilationDataBase
add_library(MyCompilationDataBase STATIC MyCompilationDataBase.h MyCompilationDataBase.cpp)
target_include_directories(MyCompilationDataBase PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(MyCompilationDataBase PRIVATE
    clang-cpp
    LLVM
)

# Lib ConfigData
add_library(ConfigData STATIC ConfigData.cpp ConfigData.hpp)
target_link_libraries(ConfigData PRIVATE
    nlohmann_json::nlohmann_json
)