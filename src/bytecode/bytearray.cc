#include "bytearray.hh"

#include <cstring>

namespace tyche {

void ByteArray::add_byte(uint32_t addr, uint8_t byte)
{
    try {
        data_.at(addr) = byte;
    } catch (std::out_of_range&) {
        data_.resize(addr + 1, 0);
        data_.at(addr) = byte;
    }
}

void ByteArray::add_int(uint32_t addr, int32_t value)
{
    uint32_t zz = ((uint32_t)(value << 1)) ^ ((uint32_t)(value >> 31));
    while (zz > 0x7F) {
        add_byte(addr++, (zz & 0x7F) | 0x80);
        zz >>= 7;
    }
    add_byte(addr, zz & 0x7F);
}

void ByteArray::add_float(uint32_t addr, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    add_byte(addr, (uint8_t)(bits));
    add_byte(addr+1, (uint8_t)(bits >> 8));
    add_byte(addr+2, (uint8_t)(bits >> 16));
    add_byte(addr+3, (uint8_t)(bits >> 24));
}

void ByteArray::add_string(uint32_t addr, std::string const& str)
{
    for (uint8_t c: str)
        add_byte(addr++, c);
    add_byte(addr, 0);
}

uint8_t ByteArray::get_byte(uint32_t addr) const
{
    return data_.at(addr);
}

std::pair<int32_t, size_t> ByteArray::get_int(uint32_t addr) const
{
    uint32_t zz = 0;
    int shift = 0;
    for (size_t i = 0; shift < 35; i++) {
        uint8_t byte = get_byte(addr++);
        zz |= (uint32_t)(byte & 0x7F) << shift;
        if (!(byte & 0x80)) {
            int32_t value = (int32_t)((zz >> 1) ^ -(zz & 1));
            return std::make_pair(value, (int)(i + 1));
        }
        shift += 7;
    }
    throw BytecodeParsingError("Error parsing int32 at position " + std::to_string(addr));
}

std::pair<float, size_t> ByteArray::get_float(uint32_t addr) const
{
    uint32_t bits = (uint32_t) get_byte(addr)
            | (uint32_t) get_byte(addr+1) << 8
            | (uint32_t) get_byte(addr+2) << 16
            | (uint32_t) get_byte(addr+3) << 24;
    float value;
    std::memcpy(&value, &bits, 4);
    return { value, 4 };
}

std::pair<std::string, size_t> ByteArray::get_string(uint32_t addr) const
{
    std::string data;
    while (char c = (char) get_byte(addr++))
        data += c;
    return { data, data.size() + 1 };
}

}