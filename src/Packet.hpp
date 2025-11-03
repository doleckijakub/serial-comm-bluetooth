#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

enum class PacketType : uint32_t
{
    STR,
    BIN,
};

struct Packet
{
    PacketType type;
    std::string str_data;
    std::vector<char> bin_data;

    std::vector<char> serialize() const
    {
        std::vector<char> out;
        uint32_t t = static_cast<uint32_t>(type);
        out.insert(out.end(), reinterpret_cast<char*>(&t),
                   reinterpret_cast<char *>(&t) +4);
        if (type == PacketType::STR)
        {
            uint32_t len = static_cast<uint32_t>(str_data.size());
            out.insert(out.end(), reinterpret_cast<char*>(&len),
                       reinterpret_cast<char *>(&len) +4);
            out.insert(out.end(), str_data.begin(), str_data.end());
        }
        else if (type == PacketType::BIN)
        {
            uint32_t len = static_cast<uint32_t>(bin_data.size());
            out.insert(out.end(), reinterpret_cast<char*>(&len),
                       reinterpret_cast<char *>(&len) +4);
            out.insert(out.end(), bin_data.begin(), bin_data.end());
        }
        return out;
    }

    static bool deserialize(const char* data, size_t size, Packet &pkt,
                            size_t &consumed)
    {
        consumed = 0;
        if (size < 8) return false;
        uint32_t t;
        std::memcpy(&t, data, 4);
        pkt.type = static_cast<PacketType>(t);
        uint32_t len;
        std::memcpy(&len, data + 4, 4);
        if (size < 8 + len) return false;
        if (pkt.type == PacketType::STR)
        {
            pkt.str_data = std::string(reinterpret_cast<const char*>(data + 8), len);
            pkt.bin_data.clear();
        }
        else if (pkt.type == PacketType::BIN)
        {
            pkt.bin_data = std::vector<char>(data + 8, data + 8 + len);
            pkt.str_data.clear();
        }
        consumed = 8 + len;
        return true;
    }
};
