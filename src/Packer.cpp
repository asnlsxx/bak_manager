#include "Packer.h"
#include "FileBase.h"
#include "FileHandler.h"
#include <fstream>
#include <iostream>
namespace fs = std::filesystem;

Packer::Packer(std::string root_path_, std::string pack_path_)
    : root_path(fs::absolute(root_path_)), bak_path(fs::absolute(pack_path_)) {
  // 获取根路径的文件夹名
  std::string folder_name = root_path.filename().string();
  // 构造备份文件路径
  bak_path = bak_path / (folder_name + ".backup");
  InitializeHandlers();
}

void Packer::InitializeHandlers() {
  handlers[fs::file_type::regular] =
      std::make_unique<RegularFileHandler>(inode_table);
  handlers[fs::file_type::directory] =
      std::make_unique<DirectoryHandler>(inode_table);
  handlers[fs::file_type::symlink] =
      std::make_unique<SymlinkHandler>(inode_table);
  // 可以在这里添加更多的文件类型处理器
}

bool Packer::Pack() {
  try {
    std::ofstream backup_file(bak_path, std::ios::binary);
    if (!backup_file) {
      throw std::runtime_error("无法创建备份文件: " + bak_path.string());
    }

    std::filesystem::current_path(root_path.parent_path());

    for (const auto &entry : fs::recursive_directory_iterator(root_path)) {
      const auto &path = entry.path();
      fs::file_type type = fs::status(path).type();
      
      if (auto it = handlers.find(type); it != handlers.end()) {
        it->second->Pack(path, backup_file);
      } else {
        std::cerr << "警告: 跳过未知文件类型: " << path.string() << std::endl;
      }
    }

    return true;
  } catch (const std::exception &e) {
    std::cerr << "打包过程出错: " << e.what() << std::endl;
    return false;
  }
}

bool Packer::Unpack() {
  // 解包功能的实现（暂时保持不变）
  return false;
}
