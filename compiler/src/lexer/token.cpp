#include "token.h"

Token::Token(TokenType type, std::string lexeme, int line, int col)
    : type(type), lexeme(std::move(lexeme)), line(line), col(col) {}

const char* Token::typeName() const {
    switch (type) {
        case TokenType::INT_LIT:        return "INT_LIT";
        case TokenType::FLOAT_LIT:      return "FLOAT_LIT";
        case TokenType::CHAR_LIT:       return "CHAR_LIT";
        case TokenType::STRING_LIT:     return "STRING_LIT";
        case TokenType::ID:             return "ID";

        case TokenType::KW_INT:         return "KW_INT";
        case TokenType::KW_FLOAT:       return "KW_FLOAT";
        case TokenType::KW_BOOL:        return "KW_BOOL";
        case TokenType::KW_CHAR:        return "KW_CHAR";
        case TokenType::KW_VOID:        return "KW_VOID";
        case TokenType::KW_AUTO:        return "KW_AUTO";
        case TokenType::KW_STRING:      return "KW_STRING";
        case TokenType::KW_CONST:       return "KW_CONST";
        case TokenType::KW_STRUCT:      return "KW_STRUCT";
        case TokenType::KW_TEMPLATE:    return "KW_TEMPLATE";
        case TokenType::KW_TYPENAME:    return "KW_TYPENAME";
        case TokenType::KW_NEW:         return "KW_NEW";
        case TokenType::KW_DELETE:      return "KW_DELETE";
        case TokenType::KW_IF:          return "KW_IF";
        case TokenType::KW_ELSE:        return "KW_ELSE";
        case TokenType::KW_WHILE:       return "KW_WHILE";
        case TokenType::KW_FOR:         return "KW_FOR";
        case TokenType::KW_BREAK:       return "KW_BREAK";
        case TokenType::KW_CONTINUE:    return "KW_CONTINUE";
        case TokenType::KW_RETURN:      return "KW_RETURN";
        case TokenType::KW_TRUE:        return "KW_TRUE";
        case TokenType::KW_FALSE:       return "KW_FALSE";
        case TokenType::KW_STATIC_CAST: return "KW_STATIC_CAST";

        case TokenType::PLUS:           return "PLUS";
        case TokenType::MINUS:          return "MINUS";
        case TokenType::STAR:           return "STAR";
        case TokenType::SLASH:          return "SLASH";
        case TokenType::PERCENT:        return "PERCENT";

        case TokenType::ASSIGN:         return "ASSIGN";
        case TokenType::PLUS_ASSIGN:    return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN:   return "MINUS_ASSIGN";
        case TokenType::STAR_ASSIGN:    return "STAR_ASSIGN";
        case TokenType::SLASH_ASSIGN:   return "SLASH_ASSIGN";
        case TokenType::PERCENT_ASSIGN: return "PERCENT_ASSIGN";
        case TokenType::AMP_ASSIGN:     return "AMP_ASSIGN";
        case TokenType::PIPE_ASSIGN:    return "PIPE_ASSIGN";

        case TokenType::EQ:             return "EQ";
        case TokenType::NEQ:            return "NEQ";
        case TokenType::LT:             return "LT";
        case TokenType::GT:             return "GT";
        case TokenType::LEQ:            return "LEQ";
        case TokenType::GEQ:            return "GEQ";

        case TokenType::AND:            return "AND";
        case TokenType::OR:             return "OR";
        case TokenType::NOT:            return "NOT";

        case TokenType::AMP:            return "AMP";
        case TokenType::PIPE:           return "PIPE";
        case TokenType::TILDE:          return "TILDE";

        case TokenType::INC:            return "INC";
        case TokenType::DEC:            return "DEC";

        case TokenType::DOT:            return "DOT";
        case TokenType::ARROW:          return "ARROW";

        case TokenType::LPAREN:         return "LPAREN";
        case TokenType::RPAREN:         return "RPAREN";
        case TokenType::LBRACE:         return "LBRACE";
        case TokenType::RBRACE:         return "RBRACE";
        case TokenType::LBRACKET:       return "LBRACKET";
        case TokenType::RBRACKET:       return "RBRACKET";
        case TokenType::SEMICOLON:      return "SEMICOLON";
        case TokenType::COMMA:          return "COMMA";
        case TokenType::COLON:          return "COLON";

        case TokenType::END:            return "END";
        case TokenType::ERR:            return "ERR";
    }
    return "UNKNOWN";
}

// Formato: TIPO("lexeme", línea:columna)
std::ostream& operator<<(std::ostream& out, const Token& tok) {
    return out << tok.typeName()
               << "(\"" << tok.lexeme << "\", "
               << tok.line << ":" << tok.col << ")";
}
