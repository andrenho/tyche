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
    return look_ahead_;
}

Token Lexer::ingest()
{
    Token t = look_ahead_;
    ingest_next_token();
    return t;
}

void Lexer::ingest_next_token()
{
    char c = source_.at(pos_);

    if (pos_ >= source_.size()) {
        look_ahead_ = { TokenType::EOF_, "" };
    } else if (c == '.') {

    } else if (c == '"') {

    } else if (isdigit(c)) {

    } else if (isalpha(c)) {

    } else if (c == '\n') {

    } else if (c != ' ' && c != '\t' && c != '\r') {
        throw AssemblyError(std::string("Unexpected character '") + c + "' (ascii: " + std::to_string((int) c) + ")");
    }
}

} // tyche
