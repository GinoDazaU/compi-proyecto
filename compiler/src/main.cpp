#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/lexer.h"

static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: no se pudo abrir '" << path << "'\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: compiler <archivo.cpp>\n";
        return 1;
    }

    Lexer lexer(readFile(argv[1]));
    for (const Token& tok : lexer.tokenize()) {
        std::cout << tok << "\n";
        if (tok.type == TokenType::ERR)
            std::cerr << "Error léxico en " << tok.line << ":" << tok.col
                      << " — caracter inesperado: '" << tok.lexeme << "'\n";
    }

    return 0;
}
