#include <emscripten.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "../../../compiler/src/lexer/lexer.h"
#include "../../../compiler/src/parser/parser.h"
#include "../../../compiler/src/parser/ast_json_printer.h"
#include "../../../compiler/src/semantic/type_checker.h"
#include "../../../compiler/src/codegen/code_generator.h"
#include "../../../compiler/src/optimizer/optimizer.h"

extern "C" {

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
    ss << "[";
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        ss << "{";
        ss << "\"type\": \"" << tok.typeName() << "\",";
        ss << "\"lexeme\": \"" << escapeJson(tok.lexeme) << "\",";
        ss << "\"line\": " << tok.line << ",";
        ss << "\"col\": " << tok.col;
        ss << "}";
        if (i + 1 < tokens.size()) ss << ",";
    }
    ss << "]";
    return ss.str();
}

EMSCRIPTEN_KEEPALIVE
const char* compile_code(const char* code_cstr, bool optimize) {
    static std::string result; // Static to keep pointer valid after return
    std::string source(code_cstr);
    
    try {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        Program* program = parser.parse();

        TypeChecker checker;
        checker.check(program);

        if (optimize) {
            optimizer::optimize(program);
        }

        std::ostringstream asm_out;
        CodeGenerator gen(asm_out);
        gen.gencode(program);
        
        // Also capture JSON AST for the frontend
        std::ostringstream json_out;
        ASTJsonPrinter printer(json_out);
        printer.visit(program);

        delete program;

        // Return a JSON string containing success, asm, and ast
        // We need to escape strings manually or just build a simple JSON
        // Since ASM can have quotes, we should properly escape it.
        std::string asm_str = asm_out.str();
        std::string ast_str = json_out.str();
        std::string tokens_str = serializeTokens(tokens);
        
        std::string escaped_asm;
        for (char c : asm_str) {
            if (c == '\n') escaped_asm += "\\n";
            else if (c == '"') escaped_asm += "\\\"";
            else if (c == '\t') escaped_asm += "\\t";
            else if (c == '\\') escaped_asm += "\\\\";
            else escaped_asm += c;
        }

        result = "{\"success\": true, \"asm\": \"" + escaped_asm + "\", \"ast\": " + ast_str + ", \"tokens\": " + tokens_str + "}";
        return result.c_str();

    } catch (const LexError& e) {
        result = std::string("{\"success\": false, \"error\": {\"type\": \"lexical\", \"line\": ") + std::to_string(e.line) + ", \"col\": " + std::to_string(e.col) + ", \"message\": \"" + escapeJson(e.what()) + "\"}}";
        return result.c_str();
    } catch (const ParseError& e) {
        result = std::string("{\"success\": false, \"error\": {\"type\": \"syntax\", \"line\": ") + std::to_string(e.line) + ", \"col\": " + std::to_string(e.col) + ", \"message\": \"" + escapeJson(e.what()) + "\"}}";
        return result.c_str();
    } catch (const SemanticError& e) {
        result = std::string("{\"success\": false, \"error\": {\"type\": \"semantic\", \"line\": ") + std::to_string(e.line) + ", \"col\": " + std::to_string(e.col) + ", \"message\": \"" + escapeJson(e.what()) + "\"}}";
        return result.c_str();
    } catch (const std::exception& e) {
        result = std::string("{\"success\": false, \"error\": {\"type\": \"server\", \"line\": 0, \"col\": 0, \"message\": \"internal error: ") + escapeJson(e.what()) + "\"}}";
        return result.c_str();
    }
}

}
