#include "bytearray.hh"

#include <cstring>
#include <cstdio>

namespace tyche {

uint8_t StaticByteArray::get_byte(uint32_t addr) const
{
    return data_.at(addr);
}

uint16_t StaticByteArray::get_uint16(uint32_t addr) const
{
    return (uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8;
}

uint32_t StaticByteArray::get_uint32(uint32_t addr) const
{
    return (uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8
            | (uint32_t) get_byte(addr+2) << 16
            | (uint32_t) get_byte(addr+3) << 24;
}

int8_t StaticByteArray::get_int8(uint32_t addr) const
{
    return std::bit_cast<int8_t>(get_byte(addr));
}

int16_t StaticByteArray::get_int16(uint32_t addr) const
{
    return (uint16_t) get_byte(addr)
            | (uint16_t) get_byte(addr+1) << 8;
}

int32_t StaticByteArray::get_int32(uint32_t addr) const
{
    return std::bit_cast<int32_t>((uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8
            | (uint32_t) get_byte(addr+2) << 16
            | (uint32_t) get_byte(addr+3) << 24);
}

float StaticByteArray::get_float(uint32_t addr) const
{
    uint32_t bits = (uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8
            | (uint32_t) get_byte(addr+2) << 16
            | (uint32_t) get_byte(addr+3) << 24;
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}

std::pair<const char*, size_t> StaticByteArray::get_string_ptr(uint32_t addr) const
{
    return { (const char *) &data_.at(addr), strlen((const char *) &data_.at(addr)) + 1 };
}

std::string StaticByteArray::hexdump() const
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