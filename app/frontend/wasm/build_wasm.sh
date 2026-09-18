#!/bin/bash
set -e

# Make sure Emscripten (emsdk) is installed and activated.
# To activate: source /path/to/emsdk/emsdk_env.sh

mkdir -p ../public/wasm

echo "Compiling to WebAssembly with Emscripten..."

em++ compiler_wrapper.cpp \
    ../../../compiler/src/lexer/token.cpp \
    ../../../compiler/src/lexer/lexer.cpp \
    ../../../compiler/src/parser/ast_json_printer.cpp \
    ../../../compiler/src/parser/parser.cpp \
    ../../../compiler/src/semantic/sem_type.cpp \
    ../../../compiler/src/semantic/type_checker.cpp \
    ../../../compiler/src/codegen/code_generator.cpp \
    ../../../compiler/src/optimizer/optimizer.cpp \
    ../../../compiler/src/optimizer/ast_walker.cpp \
    ../../../compiler/src/optimizer/constant_folder.cpp \
    ../../../compiler/src/optimizer/constant_propagator.cpp \
    ../../../compiler/src/optimizer/algebraic_simplifier.cpp \
    ../../../compiler/src/optimizer/dead_code_eliminator.cpp \
    -O3 \
    -std=c++17 \
    -fexceptions \
    -s DISABLE_EXCEPTION_CATCHING=0 \
    -s WASM=1 \
    -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createCompiler" \
    -o ../public/wasm/compiler.js

echo "Done! The WASM compiler is in app/frontend/public/wasm/"
