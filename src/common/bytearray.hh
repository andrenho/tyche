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
    void set_int8(uint32_t addr, int8_t value);
    void set_int16(uint32_t addr, int16_t value);
    void set_int32(uint32_t addr, int32_t value);
    void set_float(uint32_t addr, float value);
    void set_string(uint32_t addr, std::string const& str);
    void set_bytearray(uint32_t addr, ByteArray const& bytearray);

    void append_byte(uint8_t byte)              { set_byte(data_.size(), byte); }
    void append_uint16(uint16_t value)          { set_uint16(data_.size(), value); }
    void append_uint32(uint32_t value)          { set_uint32(data_.size(), value); }
    void append_int8(int8_t value)              { set_int8(data_.size(), value); }
    void append_int16(int16_t value)            { set_int16(data_.size(), value); }
    void append_int32(int32_t value)            { set_int32(data_.size(), value); }
    void append_float(float value)              { set_float(data_.size(), value); }
    void append_string(std::string const& str)  { set_string(data_.size(), str); }
    void append_bytearray(ByteArray const& bytearray);

    [[nodiscard]] uint8_t                        get_byte(uint32_t addr) const;
    [[nodiscard]] uint16_t                       get_uint16(uint32_t addr) const;
    [[nodiscard]] uint32_t                       get_uint32(uint32_t addr) const;
    [[nodiscard]] int8_t                         get_int8(uint32_t addr) const;
    [[nodiscard]] int16_t                        get_int16(uint32_t addr) const;
    [[nodiscard]] int32_t                        get_int32(uint32_t addr) const;
    [[nodiscard]] float                          get_float(uint32_t addr) const;
    [[nodiscard]] std::pair<const char*, size_t> get_string_ptr(uint32_t addr) const;

    [[nodiscard]] std::vector<uint8_t> const& data() const { return data_; }
    [[nodiscard]] size_t size() const { return data_.size(); }

    [[nodiscard]] std::string hexdump() const;

    friend bool operator==(ByteArray const& lhs, ByteArray const& rhs) { return lhs.data_ == rhs.data_; }

private:
    std::vector<uint8_t> data_ {};
};

}

#endif //TYCHE_BYTEARRAY_HH
