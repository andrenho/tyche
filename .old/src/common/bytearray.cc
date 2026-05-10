#include "bytearray.hh"

#include <cstring>
#include <cstdio>

namespace tyche {

void ByteArray::set_byte(uint32_t addr, uint8_t byte)
{
    if (data_.size() < (addr + 1))
        data_.resize(addr + 1, 0);
    data_.at(addr) = byte;
}

void ByteArray::set_int8(uint32_t addr, int8_t value)
{
    set_byte(addr, (uint8_t) value);
}

void ByteArray::set_int16(uint32_t addr, int16_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
}

void ByteArray::set_int32(uint32_t addr, int32_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
    set_byte(addr+2, (uint8_t) (value >> 16));
    set_byte(addr+3, (uint8_t) (value >> 24));
}

void ByteArray::set_uint16(uint32_t addr, uint16_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
}

void ByteArray::set_uint32(uint32_t addr, uint32_t value)
{
    set_byte(addr, (uint8_t) (value));
    set_byte(addr+1, (uint8_t) (value >> 8));
    set_byte(addr+2, (uint8_t) (value >> 16));
    set_byte(addr+3, (uint8_t) (value >> 24));
}

void ByteArray::set_float(uint32_t addr, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    set_byte(addr, (uint8_t) (bits));
    set_byte(addr+1, (uint8_t) (bits >> 8));
    set_byte(addr+2, (uint8_t) (bits >> 16));
    set_byte(addr+3, (uint8_t) (bits >> 24));
}

void ByteArray::set_string(uint32_t addr, std::string const& str)
{
    for (uint8_t c: str)
        set_byte(addr++, c);
    set_byte(addr, 0);
}

void ByteArray::set_bytearray(uint32_t addr, ByteArray const& bytearray)
{
    for (uint8_t byte: bytearray.data())
        set_byte(addr++, byte);
}

uint8_t ByteArray::get_byte(uint32_t addr) const
{
    return data_.at(addr);
}

uint16_t ByteArray::get_uint16(uint32_t addr) const
{
    return (uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8;
}

uint32_t ByteArray::get_uint32(uint32_t addr) const
{
    return (uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8
            | (uint32_t) get_byte(addr+2) << 16
            | (uint32_t) get_byte(addr+3) << 24;
}

int8_t ByteArray::get_int8(uint32_t addr) const
{
    return std::bit_cast<int8_t>(get_byte(addr));
}

int16_t ByteArray::get_int16(uint32_t addr) const
{
    return (uint16_t) get_byte(addr)
            | (uint16_t) get_byte(addr+1) << 8;
}

int32_t ByteArray::get_int32(uint32_t addr) const
{
    return std::bit_cast<int32_t>((uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8
            | (uint32_t) get_byte(addr+2) << 16
            | (uint32_t) get_byte(addr+3) << 24);
}

float ByteArray::get_float(uint32_t addr) const
{
    uint32_t bits = (uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8
            | (uint32_t) get_byte(addr+2) << 16
            | (uint32_t) get_byte(addr+3) << 24;
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}

std::pair<std::string, size_t> ByteArray::get_string(uint32_t addr) const
{
    std::string data;
    while (char c = (char) get_byte(addr++))
        data += c;
    return { data, data.size() + 1 };
}

void ByteArray::append_bytearray(ByteArray const& bytearray)
{
    data_.insert(data_.end(), bytearray.data().begin(), bytearray.data().end());
}

std::string ByteArray::hexdump() const
{
    auto to_hex = [](uint32_t value, size_t n_chars) -> std::string {
        char buf[15];
        snprintf(buf, sizeof buf, "%0*X", (int) n_chars, value);
        return { buf };
    };

    std::string out;
    for (size_t i = 0; i < data_.size(); ++i) {
        if (i % 16 == 0)
            out += to_hex(i, 4) + " | ";
        out += to_hex(data_.at(i), 2) + " ";
        if (i % 16 == 15)
            out += "\n";
    }
    return out + "\n";
}

}