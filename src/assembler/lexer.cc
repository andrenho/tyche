#include "lexer.hh"

#include "as_exceptions.hh"

namespace tyche::as {

std::string token_type_name(TokenType type)
{
    switch (type) {
        case TokenType::BOF: return "BOF";
        case TokenType::Directive: return "directive";
        case TokenType::Instruction: return "instruction";
        case TokenType::Integer: return "integer";
        case TokenType::Float: return "float";
        case TokenType::String: return "string";
        case TokenType::Enter: return "enter";
        case TokenType::Colon: return "colon";
        case TokenType::EOF_: return "EOF";
        default: return "???";
    }
}

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
    size_t current_line_pos = 1;
    size_t current_line = 1;

    if (pos_ >= source_.size()) {
        current_token_ = { TokenType::EOF_ };
        return;
    }

    char c = source_.at(pos_);

    TokenType type {};
    std::string stoken;
    TokenValue value = std::monostate();

    if (c == '.') {
        type = TokenType::Directive;
        stoken += '.';
        while (c = source_.at(++pos_), isalpha(c) || c == '_')
            stoken += c;
        value = stoken;
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
                throw AssemblyError("Unterminated string", current_line, pos_ - current_line_pos);
            }
            stoken += source_.at(pos_++);
        }
        value = stoken;
    } else if (isdigit(c) || c == '-') {
        type = TokenType::Integer;
        stoken += c;
        while (c = source_.at(++pos_), isdigit(c) || c == '.') {
            stoken += c;
            if (c == '.') {
                if (type == TokenType::Integer)
                    type = TokenType::Float;
                else
                    throw AssemblyError("Double point in floating point number", current_line, pos_ - current_line_pos);
            }
        }
        if (type == TokenType::Integer)
            value = std::stoi(stoken);
        else
            value = std::stof(stoken);
    } else if (isalpha(c)) {
        type = TokenType::Instruction;
        stoken += c;
        while (c = source_.at(++pos_), isalpha(c))
            stoken += c;
        value = stoken;
    } else if (c == ':') {
        type = TokenType::Colon;
        ++pos_;
    } else if (c == '\n') {
        type = TokenType::Enter;
        value = "\n";
        ++pos_;
        ++current_line;
        current_line_pos = pos_;
    } else {
        throw AssemblyError(std::string("Unexpected character '") + c + "' (ascii: " + std::to_string((int) c) + ")", current_line, pos_ - current_line_pos);
    }

    // skip ignored tokens
    while (pos_ < source_.size() && (source_.at(pos_) == ' ' || source_.at(pos_) == '\t' || source_.at(pos_) == '\r'))
        ++pos_;

    current_token_ = { .type = type, .token = value, .line = current_line, .column = pos_ - current_line_pos };
}

} // tyche
