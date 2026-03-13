#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <QByteArray>

// Length-prefix framing for raw TCP (replaces WebSocket framing).
// Wire format: [4-byte big-endian length][UTF-8 JSON payload]

static constexpr uint32_t MAX_MSG_SIZE = 65536;  // 64 KB

inline QByteArray frameMessage(const std::string& json) {
    uint32_t len = static_cast<uint32_t>(json.size());
    QByteArray frame;
    frame.reserve(4 + static_cast<int>(len));
    // Big-endian length prefix
    unsigned char header[4];
    header[0] = (len >> 24) & 0xFF;
    header[1] = (len >> 16) & 0xFF;
    header[2] = (len >>  8) & 0xFF;
    header[3] =  len        & 0xFF;
    frame.append(reinterpret_cast<const char*>(header), 4);
    frame.append(json.data(), static_cast<int>(json.size()));
    return frame;
}

// Accumulates bytes from a TCP socket and extracts complete framed messages.
class TcpFrameReader {
public:
    // Feed new data from socket->readAll(). Returns complete messages.
    std::vector<std::string> feed(const QByteArray& data) {
        buffer_.append(data);
        std::vector<std::string> messages;

        while (buffer_.size() >= 4) {
            auto* p = reinterpret_cast<const unsigned char*>(buffer_.constData());
            uint32_t len = (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16)
                         | (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);

            if (len > MAX_MSG_SIZE) {
                // Corrupt or oversized — discard buffer
                buffer_.clear();
                break;
            }

            if (buffer_.size() < static_cast<int>(4 + len))
                break;  // incomplete message

            messages.emplace_back(buffer_.constData() + 4, static_cast<size_t>(len));
            buffer_.remove(0, 4 + static_cast<int>(len));
        }
        return messages;
    }

private:
    QByteArray buffer_;
};
