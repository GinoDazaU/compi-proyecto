#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"

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

    std::string source = readFile(argv[1]);

    // Fase 1: Léxico
    Lexer lexer(source);
    std::vector<Token> tokens;
    for (const Token& tok : lexer.tokenize()) {
        if (tok.type == TokenType::ERR) {
            std::cerr << "Error léxico en " << tok.line << ":" << tok.col
                      << " — caracter inesperado: '" << tok.lexeme << "'\n";
            return 1;
        }
        tokens.push_back(tok);
    }

    // Fase 2: Parser
    try {
        Parser parser(std::move(tokens));
        Program* program = parser.parse();

        ASTPrinter printer;
        printer.visit(program);

        delete program;
    } catch (const ParseError& e) {
        std::cerr << "Error sintáctico en " << e.line << ":" << e.col
                  << " — " << e.what() << "\n";
        return 1;
    }

    return 0;
}
