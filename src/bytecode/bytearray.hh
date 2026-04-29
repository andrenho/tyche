#ifndef TYCHE_BYTEARRAY_HH
#define TYCHE_BYTEARRAY_HH

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace tyche {

class ByteArray {
public:
    ByteArray() = default;
    explicit ByteArray(std::vector<uint8_t> data) : data_(std::move(data)) {}

    void set_byte(uint32_t addr, uint8_t byte);
    void set_uint16(uint32_t addr, uint16_t value);
    void set_uint32(uint32_t addr, uint32_t value);
    void set_int(uint32_t addr, int32_t value);
    void set_float(uint32_t addr, float value);
    void set_string(uint32_t addr, std::string const& str);
    void set_bytearray(uint32_t addr, ByteArray const& bytearray);

    void append_byte(uint8_t byte) { set_byte(data_.size(), byte); }
    void append_uint16(uint16_t value) { set_uint16(data_.size(), value); }
    void append_uint32(uint32_t value) { set_uint32(data_.size(), value); }
    void append_int(int32_t value) { set_int(data_.size(), value); }
    void append_float(float value) { set_float(data_.size(), value); }
    void append_string(std::string const& str) { set_string(data_.size(), str); }
    void append_bytearray(ByteArray const& bytearray);

    [[nodiscard]] uint8_t                        get_byte(uint32_t addr) const;
    [[nodiscard]] uint16_t                       get_uint16(uint32_t addr) const;
    [[nodiscard]] uint32_t                       get_uint32(uint32_t addr) const;
    [[nodiscard]] std::pair<int32_t, size_t>     get_int(uint32_t addr) const;
    [[nodiscard]] float                          get_float(uint32_t addr) const;
    [[nodiscard]] std::pair<std::string, size_t> get_string(uint32_t addr) const;

    [[nodiscard]] std::vector<uint8_t> const& data() const { return data_; }
    [[nodiscard]] size_t size() const { return data_.size(); }

private:
    std::vector<uint8_t> data_ {};
};

class BytecodeParsingError : public std::runtime_error {
public:
    explicit BytecodeParsingError(std::string const& str) : std::runtime_error(str.c_str()) {}
};

}

#endif //TYCHE_BYTEARRAY_HH
