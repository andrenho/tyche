#ifndef TYCHE_LEXER_HH
#define TYCHE_LEXER_HH

#include <string>
#include <utility>

namespace tyche::as {

enum class TokenType { BOF, Directive, Instruction, Number, Float, String, Enter, EOF_ };

struct Token {
    TokenType   type;
    std::string token;
};

class Lexer {
public:
    explicit Lexer(std::string source) : source_(std::move(source) + "\n") { reset(); }

    void reset();
    [[nodiscard]] Token peek() const;
    [[nodiscard]] Token ingest();

private:
    const std::string source_;
    size_t pos_ = 0;
    Token look_ahead_;

    void ingest_next_token();
};

} // tyche

#endif //TYCHE_LEXER_HH
