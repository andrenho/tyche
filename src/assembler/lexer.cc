#include "lexer.hh"

#include "as_exceptions.hh"

namespace tyche::as {

void Lexer::reset()
{
    pos_ = 0;
    ingest_next_token();
}

Token Lexer::peek() const
{
    return current_token_;
}

Token Lexer::ingest()
{
    Token t = current_token_;
    ingest_next_token();
    return t;
}

void Lexer::ingest_next_token()
{
    if (pos_ >= source_.size()) {
        current_token_ = { TokenType::EOF_, "" };
        return;
    }

    char c = source_.at(pos_);

    TokenType type {};
    std::string token;

    if (c == '.') {
        type = TokenType::Directive;
        token += '.';
        while (c = source_.at(++pos_), isalpha(c) || c == '_')
            token += c;
    } else if (c == '"') {
        type = TokenType::String;
        ++pos_;
        while (true) {
            if (source_.at(pos_) == '\\') {   // TODO - improve this for special characters
                ++pos_;
            } else if (source_.at(pos_) == '"') {
                ++pos_;
                break;
            } else if (pos_ >= source_.size()) {
                throw AssemblyError("Unterminated string");
            }
            token += source_.at(pos_++);
        }
    } else if (isdigit(c) || c == '-') {
        type = TokenType::Integer;
        token += c;
        while (c = source_.at(++pos_), isdigit(c) || c == '.') {
            token += c;
            if (c == '.') {
                if (type == TokenType::Integer)
                    type = TokenType::Float;
                else
                    throw AssemblyError("Double point in floating point number");
            }
        }
    } else if (isalpha(c)) {
        type = TokenType::Instruction;
        token += c;
        while (c = source_.at(++pos_), isalpha(c))
            token += c;
    } else if (c == '\n') {
        type = TokenType::Enter;
        ++pos_;
    } else {
        throw AssemblyError(std::string("Unexpected character '") + c + "' (ascii: " + std::to_string((int) c) + ")");
    }

    // skip ignored tokens
    while (pos_ < source_.size() && (source_.at(pos_) == ' ' || source_.at(pos_) == '\t' || source_.at(pos_) == '\r'))
        ++pos_;

    current_token_ = { .type = type, .token = token };
}

} // tyche
