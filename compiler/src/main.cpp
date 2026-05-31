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

static void printUsage() {
    std::cerr << "Uso: compiler [--tokens|--ast] <archivo>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) { printUsage(); return 1; }

    std::string mode = "ast";
    std::string filepath;

    if (argc == 3) {
        std::string flag = argv[1];
        if (flag == "--tokens") mode = "tokens";
        else if (flag == "--ast") mode = "ast";
        else { printUsage(); return 1; }
        filepath = argv[2];
    } else {
        filepath = argv[1];
    }

    std::string source = readFile(filepath);

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

    if (mode == "tokens") {
        for (const Token& tok : tokens)
            std::cout << tok << "\n";
        return 0;
    }

    // Fase 2: Parser + AST
    try {
        Parser parser(tokens);
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
