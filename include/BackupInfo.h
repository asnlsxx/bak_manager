#ifndef BACKUP_INFO_H
#define BACKUP_INFO_H

#include <ctime>
#include <cstdint>

constexpr size_t BACKUP_COMMENT_SIZE = 256;

struct BackupInfo {
    time_t timestamp;                        // 时间戳
    uint32_t checksum;                       // CRC32校验和
    char comment[BACKUP_COMMENT_SIZE];       // 描述信息
    unsigned char mod;                       // 压缩、加密（暂不实现）
};

// 只保留 calculateCRC32 函数声明
uint32_t calculateCRC32(const char* data, size_t length, uint32_t crc = 0xFFFFFFFF);

#endif // BACKUP_INFO_H 