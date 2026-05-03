#ifndef TYCHE_BYTEARRAYBUILDER_HH
#define TYCHE_BYTEARRAYBUILDER_HH

#include <cstdint>
#include <string>
#include <vector>

#include "bytearray.hh"

namespace tyche {

class ByteArrayBuilder {
public:
    ByteArrayBuilder& set_byte(uint32_t addr, uint8_t byte);
    ByteArrayBuilder& set_uint16(uint32_t addr, uint16_t value);
    ByteArrayBuilder& set_uint32(uint32_t addr, uint32_t value);
    ByteArrayBuilder& set_int8(uint32_t addr, int8_t value);
    ByteArrayBuilder& set_int16(uint32_t addr, int16_t value);
    ByteArrayBuilder& set_int32(uint32_t addr, int32_t value);
    ByteArrayBuilder& set_float(uint32_t addr, float value);
    ByteArrayBuilder& set_string(uint32_t addr, std::string const& str);
    ByteArrayBuilder& set_bytearray(uint32_t addr, ByteArrayBuilder const& bytearray);

    ByteArrayBuilder& append_byte(uint8_t byte)              { set_byte(data_.size(), byte); return *this; }
    ByteArrayBuilder& append_uint16(uint16_t value)          { set_uint16(data_.size(), value); return *this; }
    ByteArrayBuilder& append_uint32(uint32_t value)          { set_uint32(data_.size(), value); return *this; }
    ByteArrayBuilder& append_int8(int8_t value)              { set_int8(data_.size(), value); return *this; }
    ByteArrayBuilder& append_int16(int16_t value)            { set_int16(data_.size(), value); return *this; }
    ByteArrayBuilder& append_int32(int32_t value)            { set_int32(data_.size(), value); return *this; }
    ByteArrayBuilder& append_float(float value)              { set_float(data_.size(), value); return *this; }
    ByteArrayBuilder& append_string(std::string const& str)  { set_string(data_.size(), str); return *this; }
    ByteArrayBuilder& append_bytearray(ByteArrayBuilder const& bytearray);

    [[nodiscard]] std::vector<uint8_t> const& data() const { return data_; }
    [[nodiscard]] size_t size() const { return data_.size(); }

    [[nodiscard]] StaticByteArray build() const;

private:
    std::vector<uint8_t> data_ {};
};

}

#endif //TYCHE_BYTEARRAYBUILDER_HH
