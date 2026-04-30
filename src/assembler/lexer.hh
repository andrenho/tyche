#ifndef TYCHE_LEXER_HH
#define TYCHE_LEXER_HH

#include <string>
#include <utility>

namespace tyche::as {

enum class TokenType { Directive, Instruction, Number, Float, String, Enter, EOF_ };

struct Token {
    TokenType   type;
    std::string token;
};

class Lexer {
public:
    explicit Lexer(std::string source) : source_(std::move(source)) {}

    void reset();
    [[nodiscard]] Token peek() const;
    [[nodiscard]] Token ingest();

private:
    std::string source_;
};

} // tyche

#endif //TYCHE_LEXER_HH
