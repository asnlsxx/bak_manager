#ifndef PACKER_H
#define PACKER_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <functional>
#include "FileHandler.h"
#include "spdlog/spdlog.h"

namespace fs = std::filesystem;

class Packer {
private:
    std::unordered_map<ino_t, std::string> inode_table;

    // 定义文件过滤器类型
    using FileFilter = std::function<bool(const fs::path&)>;
    // 初始化为默认过滤器：接受所有文件
    FileFilter filter_ = [](const fs::path&) { return true; };

public:
    Packer() = default;
    ~Packer() = default;

    void set_filter(FileFilter filter) { filter_ = filter; }
    bool Pack(const fs::path& source_path, const fs::path& target_path);
    bool Unpack(const fs::path& backup_path, const fs::path& restore_path);
    bool List(const fs::path& backup_path) const;
};

#endif // PACKER_H
