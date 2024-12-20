#include "BackupInfo.h"
#include <array>

// CRC32 多项式
constexpr uint32_t CRC32_POLYNOMIAL = 0xEDB88320;

// 计算CRC32校验和
uint32_t calculateCRC32(const char* data, size_t length, uint32_t crc) {
    static std::array<uint32_t, 256> crc32_table = []() {
        std::array<uint32_t, 256> table;
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (uint32_t j = 0; j < 8; j++) {
                c = (c >> 1) ^ ((c & 1) ? CRC32_POLYNOMIAL : 0);
            }
            table[i] = c;
        }
        return table;
    }();

    crc = ~crc;
    while (length--) {
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ static_cast<uint8_t>(*data++)];
    }
    return ~crc;
} 