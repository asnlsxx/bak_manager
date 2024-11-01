#include "FileHandler.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

std::unique_ptr<FileHandler> FileHandler::Create(const fs::path &path) {
  fs::file_type type = fs::status(path).type();
  switch (type) {
  case fs::file_type::regular:
    return std::make_unique<RegularFileHandler>(path);
  case fs::file_type::directory:
    return std::make_unique<DirectoryHandler>(path);
  case fs::file_type::symlink:
    return std::make_unique<SymlinkHandler>(path);
  default:
    return nullptr;
  }
}

std::unique_ptr<FileHandler> FileHandler::Create(const FileHeader &header) {
  auto mode = header.metadata.st_mode;
  switch (mode & S_IFMT) { // 使用 S_IFMT 提取文件类型的位
  case S_IFREG:
    return std::make_unique<RegularFileHandler>();
  case S_IFDIR:
    return std::make_unique<DirectoryHandler>();
  case S_IFLNK:
    return std::make_unique<SymlinkHandler>();
  default:
    return nullptr;
  }
}

bool FileHandler::IsHardLink() const {
  return fileheader.metadata.st_nlink > 1;
}

bool FileHandler::OpenFile(std::ios_base::openmode mode_) {
  open(fileheader.path, mode_);
  return is_open();
}

const FileHeader &FileHandler::getFileHeader() const { return fileheader; }

void FileHandler::WriteHeader(std::ofstream &backup_file) const {
  if (!backup_file) {
    throw std::runtime_error("备份文件未打开或无效");
  }

  backup_file.write(reinterpret_cast<const char *>(&fileheader),
                    sizeof(FileHeader));

  if (backup_file.fail()) {
    throw std::runtime_error("写入文件头失败");
  }
}

void RegularFileHandler::Pack(
    std::ofstream &backup_file,
    std::unordered_map<ino_t, std::string> &inode_table) {

  FileHeader fileheader = this->getFileHeader();

  if (this->IsHardLink()) {
    if (inode_table.count(fileheader.metadata.st_ino)) {
      // 如果指向的inode已打包，写入文件头
      backup_file.write(reinterpret_cast<const char *>(&fileheader),
                        sizeof(fileheader));

      // 写入固定长度的硬链接目标路径
      // TODO: 硬链接目标路径可能超过MAX_PATH_LEN
      char link_buffer[MAX_PATH_LEN] = {0};
      std::strncpy(link_buffer, inode_table[fileheader.metadata.st_ino].c_str(),
                   MAX_PATH_LEN - 1);
      backup_file.write(link_buffer, MAX_PATH_LEN);
      return;
    } else {
      // 如果指向的inode未打包，作为常规文件处理
      // TODO: 对应的inode可能不在备份文件夹中
      fileheader.metadata.st_nlink = 1;
      inode_table[fileheader.metadata.st_ino] = std::string(fileheader.path);
    }
  }

  // 写入文件头
  backup_file.write(reinterpret_cast<const char *>(&fileheader),
                    sizeof(fileheader));

  // 写入文件内容
  // TODO 是否可以直接调用 this 来写入
  std::ifstream input_file(fileheader.path, std::ios::binary);
  if (!input_file) {
    std::cerr << "无法打开文件: " << fileheader.path << std::endl;
    return;
  }
  backup_file << input_file.rdbuf();
  input_file.close();
}

void DirectoryHandler::Pack(
    std::ofstream &backup_file,
    std::unordered_map<ino_t, std::string> &inode_table) {
  this->WriteHeader(backup_file);
  // 可以在这里添加目录特定的元数据
}

void SymlinkHandler::Pack(std::ofstream &backup_file,
                          std::unordered_map<ino_t, std::string> &inode_table) {
  this->WriteHeader(backup_file);

  // 获取链接目标路径
  FileHeader fileheader = this->getFileHeader();
  std::string target_path = fs::read_symlink(fileheader.path).string();

  // 创建固定长度的buffer并填充
  // TODO: 硬链接目标路径可能超过MAX_PATH_LEN
  char link_buffer[MAX_PATH_LEN] = {0};
  std::strncpy(link_buffer, target_path.c_str(), MAX_PATH_LEN - 1);

  // 写入固定长度的链接路径
  backup_file.write(link_buffer, MAX_PATH_LEN);
}

// TODO 恢复到相同文件夹重复的文件会报错，要先removeall
void RegularFileHandler::Unpack(const FileHeader &header,
                                std::ifstream &backup_file) {
  if (header.metadata.st_nlink > 1) {
    // 处理硬链接
    char link_buffer[MAX_PATH_LEN];
    backup_file.read(link_buffer, MAX_PATH_LEN);

    // 创建硬链接
    fs::create_hard_link(link_buffer, header.path);
    return;
  }

  // 创建常规文件
  std::ofstream output_file(header.path, std::ios::binary);
  if (!output_file) {
    throw std::runtime_error("无法创建文件: " + std::string(header.path));
  }

  char buffer[4096];
  std::streamsize remaining = header.metadata.st_size;
  while (remaining > 0) {
    std::streamsize chunk_size =
        std::min(remaining, static_cast<std::streamsize>(sizeof(buffer)));
    backup_file.read(buffer, chunk_size);
    output_file.write(buffer, backup_file.gcount());
    remaining -= backup_file.gcount();
  }

  if (backup_file.fail() || output_file.fail()) {
    throw std::runtime_error("文件复制失败: " + std::string(header.path));
  }
}

void DirectoryHandler::Unpack(const FileHeader &header,
                              std::ifstream &backup_file) {
  fs::create_directories(header.path);
}

void SymlinkHandler::Unpack(const FileHeader &header,
                            std::ifstream &backup_file) {
  char link_buffer[MAX_PATH_LEN];
  backup_file.read(link_buffer, MAX_PATH_LEN);

  fs::create_symlink(link_buffer, header.path);
}
