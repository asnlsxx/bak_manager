#include "Packer.h"
#include "FileBase.h"
#include "FileHandler.h"
#include <fstream>
#include "spdlog/spdlog.h"

namespace fs = std::filesystem;

Packer::Packer(std::string source_path_, std::string target_path_)
    : source_path(fs::absolute(source_path_)), target_path(fs::absolute(target_path_)) {
  spdlog::info("开始初始化备份工具...");

  std::string folder_name = source_path.filename().string();
  target_path = target_path / (folder_name + ".backup");

  InitializeHandlers();
  spdlog::info("备份工具初始化完成");
}

void Packer::InitializeHandlers() {
  handlers[fs::file_type::regular] =
      std::make_unique<RegularFileHandler>();
  handlers[fs::file_type::directory] =
      std::make_unique<DirectoryHandler>();
  handlers[fs::file_type::symlink] =
      std::make_unique<SymlinkHandler>();
  // 可以在这里添加更多的文件类型处理器
}


bool Packer::Pack() {
  spdlog::info("开始打包: {} -> {}", source_path.string(), target_path.string());
  try {
    std::ofstream backup_file(target_path, std::ios::binary);
    if (!backup_file) {
      throw std::runtime_error("无法创建备份文件: " + target_path.string());
    }

    std::filesystem::current_path(source_path.parent_path());
    spdlog::debug("切换工作目录到: {}", source_path.parent_path().string());

    for (const auto &entry : fs::recursive_directory_iterator(source_path)) {
      const auto &path = entry.path();
      spdlog::debug("打包文件: {}", path.string());

      fs::file_type type = fs::status(path).type();

      if (auto it = handlers.find(type); it != handlers.end()) {
        it->second->Pack(path, backup_file, inode_table);
      } else {
        spdlog::warn("跳过未知文件类型: {}", path.string());
      }
    }

    return true;
  } catch (const std::exception &e) {
    spdlog::error("打包过程出错: {}", e.what());
    return false;
  }
}

fs::file_type st_mode_to_file_type(mode_t mode) {
    switch (mode & S_IFMT) {  // 使用 S_IFMT 提取文件类型的位
        case S_IFREG:  return fs::file_type::regular;
        case S_IFDIR:  return fs::file_type::directory;
        case S_IFLNK:  return fs::file_type::symlink;
        case S_IFCHR:  return fs::file_type::character;
        case S_IFBLK:  return fs::file_type::block;
        case S_IFIFO:  return fs::file_type::fifo;
        case S_IFSOCK: return fs::file_type::socket;
        default:       return fs::file_type::unknown;
    }
}

bool Packer::Unpack() {
  spdlog::info("开始解包: {} -> {}", target_path.string(), source_path.string());
  try {
    std::ifstream backup_file(target_path, std::ios::binary);
    if (!backup_file) {
      throw std::runtime_error("无法打开备份文件: " + target_path.string());
    }

    // 创建目标目录
    fs::create_directories(source_path);
    std::filesystem::current_path(source_path);

    // 读取并解包文件
    while (backup_file.peek() != EOF) {
      FileHeader header;
      backup_file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
      
      if (backup_file.eof()) break;

      spdlog::debug("解包文件: {}", header.name);
      fs::file_type type = st_mode_to_file_type(header.metadata.st_mode);

      if (auto it = handlers.find(type); it != handlers.end()) {
        it->second->Unpack(header, backup_file);
      } else {
        spdlog::warn("跳过未知文件类型: {}", header.name);
      }
    }

    spdlog::info("解包完成");
    return true;
  } catch (const std::exception &e) {
    spdlog::error("解包过程出错: {}", e.what());
    return false;
  }
}
