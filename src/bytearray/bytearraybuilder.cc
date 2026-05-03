#include "bytearraybuilder.hh"

namespace tyche {

ByteArrayBuilder& ByteArrayBuilder::set_byte(uint32_t addr, uint8_t byte)
{
    if (data_.size() < (addr + 1))
        data_.resize(addr + 1, 0);
    data_.at(addr) = byte;
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_int8(uint32_t addr, int8_t value)
{
    set_byte(addr, (uint8_t) value);
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_int16(uint32_t addr, int16_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_int32(uint32_t addr, int32_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
    set_byte(addr+2, (uint8_t) (value >> 16));
    set_byte(addr+3, (uint8_t) (value >> 24));
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_uint16(uint32_t addr, uint16_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_uint32(uint32_t addr, uint32_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
    set_byte(addr+2, (uint8_t) (value >> 16));
    set_byte(addr+3, (uint8_t) (value >> 24));
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_float(uint32_t addr, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    set_byte(addr, (uint8_t) (bits));
    set_byte(addr+1, (uint8_t) (bits >> 8));
    set_byte(addr+2, (uint8_t) (bits >> 16));
    set_byte(addr+3, (uint8_t) (bits >> 24));
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_string(uint32_t addr, std::string const& str)
{
    for (uint8_t c: str)
        set_byte(addr++, c);
    set_byte(addr, 0);
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::set_bytearray(uint32_t addr, ByteArrayBuilder const& bytearray)
{
    for (uint8_t byte: bytearray.data_)
        set_byte(addr++, byte);
    return *this;
}

ByteArrayBuilder& ByteArrayBuilder::append_bytearray(ByteArrayBuilder const& bytearray)
{
    data_.insert(data_.end(), bytearray.data_.begin(), bytearray.data_.end());
    return *this;
}

StaticByteArray ByteArrayBuilder::build() const
{
    return StaticByteArray(data_);
}

}
