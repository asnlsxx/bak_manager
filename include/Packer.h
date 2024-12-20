#ifndef PACKER_H
#define PACKER_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <functional>
#include "FileHandler.h"
#include "spdlog/spdlog.h"
#include "BackupInfo.h"

namespace fs = std::filesystem;

class Packer {
private:
    std::unordered_map<ino_t, std::string> inode_table;
    bool restore_metadata_ = false;
    BackupInfo backup_info_;

    using FileFilter = std::function<bool(const fs::path&)>;
    FileFilter filter_ = [](const fs::path&) { return true; };

public:
    Packer() {
        backup_info_.timestamp = std::time(nullptr);
        backup_info_.checksum = 0;
        backup_info_.mod = 0;
        std::memset(backup_info_.comment, 0, BACKUP_COMMENT_SIZE);
    }

    void set_filter(FileFilter filter) { filter_ = filter; }
    void set_restore_metadata(bool restore) { restore_metadata_ = restore; }
    void set_comment(const std::string& comment) {
        std::strncpy(backup_info_.comment, comment.c_str(), BACKUP_COMMENT_SIZE - 1);
    }

    bool Pack(const fs::path& source_path, const fs::path& target_path);
    bool Unpack(const fs::path& backup_path, const fs::path& restore_path);
    bool Verify(const fs::path& backup_path);
};

#endif // PACKER_H
