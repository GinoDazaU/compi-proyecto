#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_json_printer.h"
#include "semantic/type_checker.h"
#include "codegen/code_generator.h"
#include "optimizer/optimizer.h"

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
    std::cerr << "Uso: compiler [--tokens|--ast|--json|--asm] [--opt] <archivo>\n";
}

static std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}

static std::string serializeTokens(const std::vector<Token>& tokens) {
    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        ss << "    {\n";
        ss << "      \"type\": \"" << tok.typeName() << "\",\n";
        ss << "      \"lexeme\": \"" << escapeJson(tok.lexeme) << "\",\n";
        ss << "      \"line\": " << tok.line << ",\n";
        ss << "      \"col\": " << tok.col << "\n";
        ss << "    }";
        if (i + 1 < tokens.size()) {
            ss << ",";
        }
        ss << "\n";
    }
    ss << "  ]";
    return ss.str();
}

int main(int argc, char* argv[]) {
    std::string mode = "ast";
    std::string filepath;
    bool opt = false;

    // Flags en cualquier orden; el primer argumento sin '-' es el archivo.
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--tokens") mode = "tokens";
        else if (arg == "--ast")    mode = "ast";
        else if (arg == "--json")   mode = "json";
        else if (arg == "--asm")    mode = "asm";
        else if (arg == "--opt")    opt = true;
        else if (!arg.empty() && arg[0] == '-') { printUsage(); return 1; }
        else if (filepath.empty())  filepath = arg;
        else { printUsage(); return 1; }
    }
    if (filepath.empty()) { printUsage(); return 1; }

    std::string source = readFile(filepath);

    // Fase 1: Léxico
    Lexer lexer(source);
    std::vector<Token> tokens;
    
    if (mode == "json") {
        for (const Token& tok : lexer.tokenize()) {
            if (tok.type == TokenType::ERR) {
                std::cout << "{\n"
                          << "  \"success\": false,\n"
                          << "  \"error\": {\n"
                          << "    \"type\": \"lexical\",\n"
                          << "    \"line\": " << tok.line << ",\n"
                          << "    \"col\": " << tok.col << ",\n"
                          << "    \"message\": \"unexpected character: '" << escapeJson(tok.lexeme) << "'\"\n"
                          << "  }\n"
                          << "}\n";
                return 0;
            }
            tokens.push_back(tok);
        }
    } else {
        for (const Token& tok : lexer.tokenize()) {
            if (tok.type == TokenType::ERR) {
                std::cerr << "lexical error at " << tok.line << ":" << tok.col
                          << ": unexpected character '" << tok.lexeme << "'\n";
                return 1;
            }
            tokens.push_back(tok);
        }
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

        // Fase 3: Semántico
        TypeChecker checker;
        checker.check(program);

        // Fase 3.5: Optimización opcional sobre el AST (--opt)
        if (opt) optimizer::optimize(program);

        if (mode == "json") {
            std::stringstream ss;
            ASTJsonPrinter printer(ss);
            printer.visit(program);
            
            std::cout << "{\n"
                      << "  \"success\": true,\n"
                      << "  \"tokens\": " << serializeTokens(tokens) << ",\n"
                      << "  \"ast\": " << ss.str() << "\n"
                      << "}\n";
        } else if (mode == "asm") {
            // Fase 4: Generación de código x86-64
            CodeGenerator gen(std::cout);
            gen.gencode(program);
        } else {
            // modo --ast: volcado del AST en JSON
            ASTJsonPrinter printer(std::cout);
            printer.visit(program);
            std::cout << "\n";
        }
        delete program;
    } catch (const ParseError& e) {
        if (mode == "json") {
            std::cout << "{\n"
                      << "  \"success\": false,\n"
                      << "  \"error\": {\n"
                      << "    \"type\": \"syntax\",\n"
                      << "    \"line\": " << e.line << ",\n"
                      << "    \"col\": " << e.col << ",\n"
                      << "    \"message\": \"" << escapeJson(e.what()) << "\"\n"
                      << "  }\n"
                      << "}\n";
            return 0;
        } else {
            std::cerr << "syntax error at " << e.line << ":" << e.col
                      << ": " << e.what() << "\n";
            return 1;
        }
    } catch (const SemanticError& e) {
        if (mode == "json") {
            std::cout << "{\n"
                      << "  \"success\": false,\n"
                      << "  \"error\": {\n"
                      << "    \"type\": \"semantic\",\n"
                      << "    \"line\": " << e.line << ",\n"
                      << "    \"col\": " << e.col << ",\n"
                      << "    \"message\": \"" << escapeJson(e.what()) << "\"\n"
                      << "  }\n"
                      << "}\n";
            return 0;
        } else {
            std::cerr << "semantic error at " << e.line << ":" << e.col
                      << ": " << e.what() << "\n";
            return 1;
        }
    }

    return 0;
}
