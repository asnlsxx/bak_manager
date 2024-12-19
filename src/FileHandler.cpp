#include "FileHandler.h"
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>
FileHandler::FileHandler(const fs::path &filepath) : std::fstream() {
  // 获取文件元数据
  struct stat file_stat;
  if (lstat(filepath.c_str(), &file_stat) != 0) {
    throw std::runtime_error("无法获取文件信息: " + filepath.string());
  }
  // 使用相对路径
  std::snprintf(fileheader.path, MAX_PATH_LEN, "%s", filepath.c_str());
  fileheader.metadata = file_stat;
}

std::unique_ptr<FileHandler> FileHandler::Create(const fs::path &path) {
  // 使用 symlink_status 而不是 status 来获取链接本身的类型
  fs::file_type type = fs::symlink_status(path).type();
  switch (type) {
  case fs::file_type::symlink:
    return std::make_unique<SymlinkHandler>(path);
  case fs::file_type::regular:
    return std::make_unique<RegularFileHandler>(path);
  case fs::file_type::directory:
    return std::make_unique<DirectoryHandler>(path);
  default:
    return nullptr;
  }
}

std::unique_ptr<FileHandler> FileHandler::Create(const FileHeader &header) {
  auto mode = header.metadata.st_mode;
  switch (mode & S_IFMT) {
  case S_IFLNK:
    return std::make_unique<SymlinkHandler>(header);
  case S_IFREG:
    return std::make_unique<RegularFileHandler>(header);
  case S_IFDIR:
    return std::make_unique<DirectoryHandler>(header);
  default:
    return nullptr;
  }
}

bool FileHandler::IsHardLink() const {
  return fileheader.metadata.st_nlink > 1;
}

bool FileHandler::OpenFile(std::ios_base::openmode mode_) {
  this->open(this->fileheader.path, mode_);
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

  FileHeader header = this->getFileHeader();

  if (this->IsHardLink()) {
    if (inode_table.count(header.metadata.st_ino)) {
      // 如果指向的inode已打包，写入文件头
      backup_file.write(reinterpret_cast<const char *>(&header),
                        sizeof(header));

      // 写入固定长度的硬链接目标路径
      // TODO: 硬链接目标路径可能超过MAX_PATH_LEN
      char link_buffer[MAX_PATH_LEN] = {0};
      std::strncpy(link_buffer, inode_table[header.metadata.st_ino].c_str(),
                   MAX_PATH_LEN - 1);
      backup_file.write(link_buffer, MAX_PATH_LEN);
      return;
    } else {
      // 如果指向的inode未打包，作为常规文件处理
      // TODO: 对应的inode可能不在备份文件夹中
      header.metadata.st_nlink = 1;
      inode_table[header.metadata.st_ino] = std::string(header.path);
    }
  }

  // 写入文件头
  backup_file.write(reinterpret_cast<const char *>(&header),
                    sizeof(header));

  // 写入文件内容
  if (!this->OpenFile()) {
    throw std::runtime_error("无法打开文件: " + std::string(header.path));
  }
  backup_file << this->rdbuf();
  this->close();
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
  FileHeader header = this->getFileHeader();

  std::string target_path = fs::read_symlink(header.path).string();
  // 创建固定长度的buffer并填充
  char link_buffer[MAX_PATH_LEN] = {0};
  std::strncpy(link_buffer, target_path.c_str(), MAX_PATH_LEN - 1);

  // 写入固定长度的链接路径
  backup_file.write(link_buffer, MAX_PATH_LEN);
}

// TODO 恢复到相同文件夹重复的文件会报错，要先removeall
void RegularFileHandler::Unpack(std::ifstream &backup_file) {
  FileHeader header = this->getFileHeader();
  if (header.metadata.st_nlink > 1) {
    // 处理硬链接
    char link_buffer[MAX_PATH_LEN];
    backup_file.read(link_buffer, MAX_PATH_LEN);

    // 创建硬链接，使用当前目录作为基准
    fs::path target_path = fs::current_path() / link_buffer;
    fs::path link_path = fs::current_path() / header.path;
    
    // 确保父目录存在
    fs::create_directories(link_path.parent_path());
    fs::create_hard_link(target_path, link_path);
    return;
  }

  // 创建常规文件，使用当前目录作为基准
  fs::path output_path = fs::current_path() / header.path;
  fs::create_directories(output_path.parent_path());
  
  std::ofstream output_file(output_path, std::ios::binary);
  if (!output_file) {
    throw std::runtime_error("无法创建文件: " + output_path.string());
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

void DirectoryHandler::Unpack(std::ifstream &backup_file) {
  FileHeader header = this->getFileHeader();
  fs::path dir_path = fs::current_path() / header.path;
  fs::create_directories(dir_path);
}

void SymlinkHandler::Unpack(std::ifstream &backup_file) {
  FileHeader header = this->getFileHeader();
  char link_buffer[MAX_PATH_LEN];
  backup_file.read(link_buffer, MAX_PATH_LEN);
  
  fs::path link_path = fs::current_path() / header.path;
  fs::create_directories(link_path.parent_path());
  fs::create_symlink(link_buffer, link_path);
}
