#ifndef PACKER_H
#define PACKER_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <memory>
#include "FileHandler.h"
#include "spdlog/spdlog.h"

namespace fs = std::filesystem;

class Packer {
private:
  fs::path source_path;      // 源路径（打包时是源目录，解包时是备份文件）
  fs::path target_path;      // 目标路径（打包时是备份文件，解包时是目标目录）
  std::unordered_map<ino_t, std::string> inode_table;

public:
  Packer(std::string source_path_, std::string target_path_);
  ~Packer() = default;

  bool Pack();
  bool Unpack();
};

#endif // PACKER_H
