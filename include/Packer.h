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
#include "AES.h"

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
    /**
     * @brief 构造函数，初始化备份头部信息
     */
    Packer() {
        backup_header_.timestamp = std::time(nullptr);
        backup_header_.checksum = 0;
        backup_header_.mod = 0;
        std::memset(backup_header_.comment, 0, COMMENT_SIZE);
    }

    /**
     * @brief 设置文件过滤器
     * @param filter 用于过滤文件的函数对象
     */
    void set_filter(FileFilter filter) { filter_ = filter; }

    /**
     * @brief 设置是否恢复文件元数据
     * @param restore true表示恢复元数据，false表示不恢复
     */
    void set_restore_metadata(bool restore) { restore_metadata_ = restore; }

    /**
     * @brief 设置备份文件的注释信息
     * @param comment 注释内容
     */
    void set_comment(const std::string& comment) {
        std::strncpy(backup_header_.comment, comment.c_str(), COMMENT_SIZE - 1);
    }

    /**
     * @brief 设置是否压缩备份文件
     * @param compress true表示启用压缩，false表示不压缩
     */
    void set_compress(bool compress) { 
        compress_ = compress;
        if (compress) {
            backup_header_.mod |= MOD_COMPRESSED;
        } else {
            backup_header_.mod &= ~MOD_COMPRESSED;
        }
    }

    /**
     * @brief 设置是否加密备份文件
     * @param encrypt true表示启用加密，false表示不加密
     * @param password 加密密码
     */
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

    /**
     * @brief 执行文件备份操作
     * @param source_path 源文件或目录路径
     * @param target_path 备份文件保存路径
     * @return 备份是否成功
     */
    bool Pack(const fs::path& source_path, const fs::path& target_path);

    /**
     * @brief 执行备份文件还原操作
     * @param backup_path 备份文件路径
     * @param restore_path 还原目标路径
     * @return 还原是否成功
     */
    bool Unpack(const fs::path& backup_path, const fs::path& restore_path);

    /**
     * @brief 验证备份文件的完整性
     * @param backup_path 备份文件路径
     * @return 验证是否通过
     */
    bool Verify(const fs::path& backup_path);
};

#endif // PACKER_H
