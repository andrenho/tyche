#ifndef TYCHE_LEXER_HH
#define TYCHE_LEXER_HH

#include <string>
#include <utility>
#include <variant>

namespace tyche::as {

enum class TokenType {
    BOF, Directive, Instruction, Integer, Float, String, Enter, Colon, EOF_
};

using TokenValue = std::variant<std::monostate, int, float, std::string>;

struct Token {
    TokenType   type;
    TokenValue  token = std::monostate();
    size_t      line = 0;
    size_t      column = 0;

    friend bool operator==(Token const& lhs, Token const& rhs) { return std::tie(lhs.type, lhs.token) == std::tie(rhs.type, rhs.token); }
};

std::string token_type_name(TokenType type);

class Lexer {
public:
    explicit Lexer(std::string source) : source_(std::move(source)) { reset(); }

    void reset();
    [[nodiscard]] Token peek() const;
    [[nodiscard]] Token ingest();

private:
    const std::string source_;
    size_t pos_ = 0;
    Token current_token_ { TokenType::BOF };

    void ingest_next_token();
};

} // tyche

#endif //TYCHE_LEXER_HH
