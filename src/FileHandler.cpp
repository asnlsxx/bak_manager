#include "FileHandler.h"
#include "FileBase.h"
#include <cstring>
#include <algorithm>
#include <vector>

void RegularFileHandler::Pack(const fs::path &path,
                              std::ofstream &backup_file) {
  FileBase file(path.string());
  FileHeader fileheader = file.getFileHeader();

  if (file.IsHardLink()) {
    if (inode_table_.count(fileheader.metadata.st_ino)) {
      // 如果指向的inode已打包，写入文件头
      backup_file.write(reinterpret_cast<const char *>(&fileheader),
                        sizeof(fileheader));

      // 写入固定长度的硬链接目标路径
      // TODO: 硬链接目标路径可能超过MAX_PATH_LEN
      char link_buffer[MAX_PATH_LEN] = {0};
      std::strncpy(link_buffer,
                   inode_table_[fileheader.metadata.st_ino].c_str(),
                   MAX_PATH_LEN - 1);
      backup_file.write(link_buffer, MAX_PATH_LEN);
      return;
    } else {
      // 如果指向的inode未打包，作为常规文件处理
      fileheader.metadata.st_nlink = 1;
      inode_table_[fileheader.metadata.st_ino] = path.string();
    }
  }

  // 写入文件头
  backup_file.write(reinterpret_cast<const char *>(&fileheader),
                    sizeof(fileheader));

  // 写入文件内容
  std::ifstream input_file(path, std::ios::binary);
  if (!input_file) {
    std::cerr << "无法打开文件: " << path << std::endl;
    return;
  }
  backup_file << input_file.rdbuf();
  input_file.close();
}

void DirectoryHandler::Pack(const fs::path &path, std::ofstream &backup_file) {
  FileBase dir(path.string());
  dir.WriteHeader(backup_file);
  // 可以在这里添加目录特定的元数据
}

void SymlinkHandler::Pack(const fs::path &path, std::ofstream &backup_file) {
  FileBase symlink(path.string());
  symlink.WriteHeader(backup_file);

  // 获取链接目标路径
  std::string target_path = fs::read_symlink(path).string();

  // 创建固定长度的buffer并填充
  // TODO: 硬链接目标路径可能超过MAX_PATH_LEN
  char link_buffer[MAX_PATH_LEN] = {0};
  std::strncpy(link_buffer, target_path.c_str(), MAX_PATH_LEN - 1);

  // 写入固定长度的链接路径
  backup_file.write(link_buffer, MAX_PATH_LEN);
}

void RegularFileHandler::Unpack(const FileHeader &header, std::ifstream &backup_file) {
  if (header.metadata.st_nlink > 1) {
    // 处理硬链接
    char link_buffer[MAX_PATH_LEN];
    backup_file.read(link_buffer, MAX_PATH_LEN);
    
    // 创建硬链接
    fs::create_hard_link(link_buffer, header.name);
    return;
  }

  // 创建常规文件
  std::ofstream output_file(header.name, std::ios::binary);
  if (!output_file) {
    throw std::runtime_error("无法创建文件: " + std::string(header.name));
  }

  // 分配缓冲区并读取文件内容
  std::vector<char> buffer(header.metadata.st_size);
  backup_file.read(buffer.data(), header.metadata.st_size);
  
  if (backup_file.gcount() != header.metadata.st_size) {
    throw std::runtime_error("文件读取不完整: " + std::string(header.name));
  }

  // 写入文件内容
  output_file.write(buffer.data(), header.metadata.st_size);
  
  if (output_file.fail()) {
    throw std::runtime_error("文件写入失败: " + std::string(header.name));
  }

  // 设置文件权限
  fs::permissions(header.name, 
                 static_cast<fs::perms>(header.metadata.st_mode), 
                 fs::perm_options::replace);
}

void DirectoryHandler::Unpack(const FileHeader &header, std::ifstream &backup_file) {
  fs::create_directories(header.name);
  fs::permissions(header.name, 
                 static_cast<fs::perms>(header.metadata.st_mode),
                 fs::perm_options::replace);
}

void SymlinkHandler::Unpack(const FileHeader &header, std::ifstream &backup_file) {
  char link_buffer[MAX_PATH_LEN];
  backup_file.read(link_buffer, MAX_PATH_LEN);
  
  fs::create_symlink(link_buffer, header.name);
}
