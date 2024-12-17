#ifndef PACKER_H
#define PACKER_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <memory>
#include "FileHandler.h"
#include "spdlog/spdlog.h"
#include <functional>

namespace fs = std::filesystem;

class Packer {
private:
  fs::path source_path;      
  fs::path target_path;      
  std::unordered_map<ino_t, std::string> inode_table;

  // 定义文件过滤器类型
  using FileFilter = std::function<bool(const fs::path&)>;
  FileFilter filter_;

public:
  Packer(std::string source_path_, std::string target_path_);
  ~Packer() = default;

  // 设置过滤器的方法
  void set_filter(FileFilter filter) { filter_ = filter; }

  bool Pack();
  bool Unpack();
};

#endif // PACKER_H
