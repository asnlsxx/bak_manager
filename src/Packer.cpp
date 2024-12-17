#include "Packer.h"
#include "FileHandler.h"
#include <fstream>
#include "spdlog/spdlog.h"

namespace fs = std::filesystem;

Packer::Packer(std::string source_path_, std::string target_path_)
    : source_path(fs::absolute(source_path_)), target_path(fs::absolute(target_path_)) {
  spdlog::info("开始初始化备份工具...");

  std::string folder_name = source_path.filename().string();
  // TODO 直接加后缀？
  target_path = target_path / (folder_name + ".backup");

  spdlog::info("备份工具初始化完成");
}



bool Packer::Pack() {
  // 如果没有设置filter,使用默认的接受所有文件的过滤器
  if (!filter_) {
    filter_ = [](const fs::path&) { return true; };
  }

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
      
      // 使用过滤器判断是否需要打包该文件
      if (!filter_(path)) {
        spdlog::debug("跳过文件: {}", path.string());
        continue;
      }

      spdlog::debug("打包文件: {}", path.string());

      if (auto handler = FileHandler::Create(path)) {
        handler->Pack(backup_file, inode_table);
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
      
      spdlog::debug("解包文件: {}", header.path);

      if (auto handler = FileHandler::Create(header)) {
        handler->Unpack(backup_file);
      } else {
        spdlog::warn("跳过未知文件类型: {}", std::string(header.path));
      }
    }

    spdlog::info("解包完成");
    return true;
  } catch (const std::exception &e) {
    spdlog::error("解包过程出错: {}", e.what());
    return false;
  }
}
