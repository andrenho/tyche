#ifndef TYCHE_BYTEARRAY_HH
#define TYCHE_BYTEARRAY_HH

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace tyche {

class ByteArray {
public:
    void add_byte(uint32_t addr, uint8_t byte);
    void add_int(uint32_t addr, int32_t value);
    void add_float(uint32_t addr, float value);
    void add_string(uint32_t addr, std::string const& str);

    [[nodiscard]] uint8_t                        get_byte(uint32_t addr) const;
    [[nodiscard]] std::pair<int32_t, size_t>     get_int(uint32_t addr) const;
    [[nodiscard]] std::pair<float, size_t>       get_float(uint32_t addr) const;
    [[nodiscard]] std::pair<std::string, size_t> get_string(uint32_t addr) const;

    [[nodiscard]] std::vector<uint8_t> const& data() const { return data_; }

private:
    std::vector<uint8_t> data_;
};

class BytecodeParsingError : public std::runtime_error {
public:
    explicit BytecodeParsingError(std::string const& str) : std::runtime_error(str.c_str()) {}
};

}

#endif //TYCHE_BYTEARRAY_HH
