#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/ast_printer.h"
#include "parser/ast_json_printer.h"
#include "semantic/type_checker.h"

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
    std::cerr << "Uso: compiler [--tokens|--ast|--json] <archivo>\n";
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
    if (argc < 2 || argc > 3) { printUsage(); return 1; }

    std::string mode = "ast";
    std::string filepath;

    if (argc == 3) {
        std::string flag = argv[1];
        if (flag == "--tokens") mode = "tokens";
        else if (flag == "--ast") mode = "ast";
        else if (flag == "--json") mode = "json";
        else { printUsage(); return 1; }
        filepath = argv[2];
    } else {
        filepath = argv[1];
    }

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

        if (mode == "json") {
            std::stringstream ss;
            ASTJsonPrinter printer(ss);
            printer.visit(program);
            
            std::cout << "{\n"
                      << "  \"success\": true,\n"
                      << "  \"tokens\": " << serializeTokens(tokens) << ",\n"
                      << "  \"ast\": " << ss.str() << "\n"
                      << "}\n";
        } else {
            ASTPrinter printer;
            printer.visit(program);
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
