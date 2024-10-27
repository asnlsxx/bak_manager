#ifndef PACKER_H
#define PACKER_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <fstream>
#include "FileBase.h"

namespace fs = std::filesystem;

class Packer {
private:
  fs::path root_path; // 需要备份或恢复的文件路径
  fs::path bak_path;  // 打包文件的路径
  std::unordered_map<ino_t, std::string> inode_table;

public:
  Packer(std::string root_path_, std::string pack_path_);
  ~Packer();

  bool Pack();

  bool Unpack();
};
#endif // PACKER_H
