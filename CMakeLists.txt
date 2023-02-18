cmake_minimum_required(VERSION 3.10)
project(pas_compiler)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Wall -pedantic -g")

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)


# Now build our tools
add_executable(pas_compiler main.cpp Lexan.cpp Lexan.h Parser.cpp Parser.h AbstractSyntaxTree.cpp AbstractSyntaxTree.h codegen.cpp)

# Link against LLVM libraries
# target_link_libraries(pas_compiler ${llvm_libs})

set(llvmCodeGenLibs XCoreCodeGen SystemZCodeGen SparcCodeGen PowerPCCodeGen NVPTXCodeGen MSP430CodeGen MipsCodeGen LanaiCodeGen HexagonCodeGen BPFCodeGen ARMCodeGen AMDGPUCodeGen AArch64CodeGen X86CodeGen CodeGen SystemZAsmParser SparcAsmParser PowerPCAsmParser MipsAsmParser LanaiAsmParser HexagonAsmParser BPFAsmParser ARMAsmParser AMDGPUAsmParser AArch64AsmParser X86AsmParser AsmParser)

llvm_map_components_to_libnames(llvm_libs support core irreader target ${llvmCodeGenLibs})
target_link_libraries(pas_compiler ${llvm_libs})

