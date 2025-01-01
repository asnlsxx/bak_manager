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
#include "aes.h"

namespace fs = std::filesystem;

class Packer {
private:
    static constexpr size_t COMMENT_SIZE = 256;
    struct BackupHeader {
        time_t timestamp;                // 时间戳
        uint32_t checksum;              // CRC32校验和
        char comment[COMMENT_SIZE];      // 描述信息
        unsigned char mod;               // 压缩、加密标志位
    };

    // 定义标志位
    static constexpr unsigned char MOD_COMPRESSED = 0x01;  // 0000 0001
    static constexpr unsigned char MOD_ENCRYPTED = 0x02;   // 0000 0010

    std::unordered_map<ino_t, std::string> inode_table;
    bool restore_metadata_ = false;
    bool compress_ = false;              // 是否启用压缩
    bool encrypt_ = false;               // 是否启用加密
    std::unique_ptr<AESModule> aes_;    // AES加密模块
    BackupHeader backup_header_;

    using FileFilter = std::function<bool(const fs::path&)>;
    FileFilter filter_ = [](const fs::path&) { return true; };

    // 私有辅助函数
    uint32_t calculateCRC32(const char* data, size_t length, uint32_t crc = 0xFFFFFFFF) const;
    bool PackToFile(const fs::path& source_path, const fs::path& target_path);
    bool UnpackFromFile(const fs::path& backup_path, const fs::path& restore_path);

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
    void set_compress(bool compress) { 
        compress_ = compress;
        if (compress) {
            backup_header_.mod |= MOD_COMPRESSED;
        } else {
            backup_header_.mod &= ~MOD_COMPRESSED;
        }
    }
    void set_encrypt(bool encrypt, const std::string& password) {
        encrypt_ = encrypt;
        if (encrypt) {
            aes_ = std::make_unique<AESModule>(password);
            backup_header_.mod |= MOD_ENCRYPTED;
        } else {
            aes_.reset();
            backup_header_.mod &= ~MOD_ENCRYPTED;
        }
    }

    bool Pack(const fs::path& source_path, const fs::path& target_path);
    bool Unpack(const fs::path& backup_path, const fs::path& restore_path);
    bool Verify(const fs::path& backup_path);
};

#endif // PACKER_H
