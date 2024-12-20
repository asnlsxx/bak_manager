#ifndef PACKER_H
#define PACKER_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <functional>
#include <ctime>
#include <cstdint>
#include "FileHandler.h"
#include "spdlog/spdlog.h"

namespace fs = std::filesystem;

class Packer {
private:
    static constexpr size_t COMMENT_SIZE = 256;
    struct BackupHeader {
        time_t timestamp;                // 时间戳
        uint32_t checksum;              // CRC32校验和
        char comment[COMMENT_SIZE];      // 描述信息（暂不实现）
        unsigned char mod;               // 压缩、加密（暂不实现）
    };

    std::unordered_map<ino_t, std::string> inode_table;
    bool restore_metadata_ = false;
    BackupHeader backup_header_;

    using FileFilter = std::function<bool(const fs::path&)>;
    FileFilter filter_ = [](const fs::path&) { return true; };

    uint32_t calculateCRC32(const char* data, size_t length, uint32_t crc = 0xFFFFFFFF) const;

public:
    Packer() {
        backup_header_.timestamp = std::time(nullptr);
        backup_header_.checksum = 0;
        backup_header_.mod = 0;
        std::memset(backup_header_.comment, 0, COMMENT_SIZE);
    }

    void set_filter(FileFilter filter) { filter_ = filter; }
    void set_restore_metadata(bool restore) { restore_metadata_ = restore; }
    void set_comment(const std::string& comment) {
        std::strncpy(backup_header_.comment, comment.c_str(), COMMENT_SIZE - 1);
    }

    bool Pack(const fs::path& source_path, const fs::path& target_path);
    bool Unpack(const fs::path& backup_path, const fs::path& restore_path);
    bool Verify(const fs::path& backup_path);
};

#endif // PACKER_H
