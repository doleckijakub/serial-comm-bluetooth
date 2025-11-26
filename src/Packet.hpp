#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

struct Header {
    uint16_t totalChunks;
    uint16_t chunkIndex;
    uint16_t payloadSize;
};

struct Packet
{
    Header header;
    std::vector<char> payload;

    std::vector<char> serialize() const {
        std::vector<char> buffer(sizeof(Header) + payload.size());
        std::memcpy(buffer.data(), &header, sizeof(Header));
        std::memcpy(buffer.data() + sizeof(Header), payload.data(), payload.size());
        return buffer;
    }

    static bool deserialize(const char* data, size_t size, Packet& out, size_t& consumed)
    {
        consumed = 0;

        if (size < sizeof(Header))
            return false;

        std::memcpy(&out.header, data, sizeof(Header));

        const size_t totalSize = sizeof(Header) + out.header.payloadSize;
        if (size < totalSize)
            return false;

        out.payload.assign(
            reinterpret_cast<const char*>(data + sizeof(Header)),
            reinterpret_cast<const char*>(data + sizeof(Header) + out.header.payloadSize)
        );

        consumed = totalSize;

        return true;
    }
};

static std::vector<Packet> chunk(const std::vector<char>& data) {
    size_t maxPayload = 2048;
    size_t totalChunks = (data.size() + maxPayload - 1) / maxPayload;
    std::vector<Packet> chunks;

    for (size_t i = 0; i < totalChunks; ++i) {
        size_t start = i * maxPayload;
        size_t end = std::min(start + maxPayload, data.size());

        Packet pkt;
        pkt.header.totalChunks = static_cast<uint16_t>(totalChunks);
        pkt.header.chunkIndex = static_cast<uint16_t>(i);
        pkt.payload.assign(data.begin() + start, data.begin() + end);
        pkt.header.payloadSize = static_cast<uint16_t>(pkt.payload.size());

        chunks.push_back(pkt);
    }

    return chunks;
}
