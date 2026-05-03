#ifndef TYCHE_BYTEARRAY_HH
#define TYCHE_BYTEARRAY_HH

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace tyche {

class StaticByteArray {
public:
    explicit StaticByteArray(std::vector<uint8_t> const& data) : data_(data) {}

    explicit StaticByteArray(StaticByteArray const& ba) : data_(ba.data()) {}

    // not assignable or moveable
    StaticByteArray(StaticByteArray&&) = delete;
    StaticByteArray& operator=(StaticByteArray const&) = delete;
    StaticByteArray& operator=(StaticByteArray&&) = delete;

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

    friend bool operator==(StaticByteArray const& lhs, StaticByteArray const& rhs) { return lhs.data_ == rhs.data_; }

private:
    const std::vector<uint8_t> data_ {};
};

}

#endif //TYCHE_BYTEARRAY_HH
