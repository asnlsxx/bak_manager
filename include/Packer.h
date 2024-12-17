#ifndef PACKER_H
#define PACKER_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <memory>
#include "FileHandler.h"
#include "spdlog/spdlog.h"
#include "cmdline.h"
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

  // 解析命令行参数并设置过滤器
  void parse_and_set_filter(const cmdline::parser& parser);

public:
  Packer(const cmdline::parser& parser);
  ~Packer() = default;

  bool Pack();
  bool Unpack();
  bool List() const;  // 新增查看备份文件信息的功能
};

#endif // PACKER_H
