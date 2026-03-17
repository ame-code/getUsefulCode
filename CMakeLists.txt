cmake_minimum_required(VERSION 3.20)

project(GetUsefulCode LANGUAGES CXX)

#set(CMAKE_C_COMPILER /opt/llvm-tooling/bin/clang)
#set(CMAKE_CXX_COMPILER /opt/llvm-tooling/bin/clang++)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)

add_subdirectory(include) 

add_executable(get_useful_code getUsefulCode.cpp
)
target_include_directories(get_useful_code PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
    include
)
target_link_libraries(get_useful_code PRIVATE
    clang-cpp
    LLVM
    LLVMSupport
    GetValidCodeFrontendActionFactory
    MyCompilationDataBase
    nlohmann_json::nlohmann_json
    ConfigData
)

install(TARGETS get_useful_code DESTINATION bin)
install(FILES config.json DESTINATION etc/getUsefulCode)