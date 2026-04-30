#ifndef TYCHE_ASSEMBLER_HH
#define TYCHE_ASSEMBLER_HH

#include <string>

#include "../common/bytearray.hh"

namespace tyche::as {

class Assembler {
public:
    explicit Assembler(std::string const& source);

    [[nodiscard]] ByteArray assemble() const;
};

} // tyche

#endif //TYCHE_ASSEMBLER_HH
