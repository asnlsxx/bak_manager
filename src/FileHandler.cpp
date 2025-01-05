// 实现文件处理器的核心功能
// 包括普通文件、目录、符号链接和管道文件的打包和解包操作
// 支持文件元数据的保存和恢复

#include "FileHandler.h"
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>

// 根据文件路径构造处理器
// 获取文件元数据并初始化文件头
FileHandler::FileHandler(const fs::path &filepath) : std::fstream() {
  struct stat file_stat;
  if (lstat(filepath.c_str(), &file_stat) != 0) {
    throw std::runtime_error("无法获取文件信息: " + filepath.string());
  }
  // 使用相对路径存储,便于还原时的路径处理
  std::snprintf(fileheader.path, MAX_PATH_LEN, "%s", filepath.c_str());
  fileheader.metadata = file_stat;
}

// 根据文件类型创建对应的处理器
// 使用 symlink_status 以正确处理符号链接
std::unique_ptr<FileHandler> FileHandler::Create(const fs::path &path) {
  fs::file_type type = fs::symlink_status(path).type();
  switch (type) {
  case fs::file_type::symlink:
    return std::make_unique<SymlinkHandler>(path);
  case fs::file_type::regular:
    return std::make_unique<RegularFileHandler>(path);
  case fs::file_type::directory:
    return std::make_unique<DirectoryHandler>(path);
  case fs::file_type::fifo:
    return std::make_unique<FIFOHandler>(path);
  default:
    return nullptr;
  }
}

// 根据文件头信息创建对应的处理器
// 用于从备份文件还原时创建正确的处理器类型
std::unique_ptr<FileHandler> FileHandler::Create(const FileHeader &header) {
  auto mode = header.metadata.st_mode;
  switch (mode & S_IFMT) {
  case S_IFLNK:
    return std::make_unique<SymlinkHandler>(header);
  case S_IFREG:
    return std::make_unique<RegularFileHandler>(header);
  case S_IFDIR:
    return std::make_unique<DirectoryHandler>(header);
  case S_IFIFO:
    return std::make_unique<FIFOHandler>(header);
  default:
    return nullptr;
  }
}

// 检查文件是否为硬链接
bool FileHandler::IsHardLink() const {
  return fileheader.metadata.st_nlink > 1;
}

// 打开文件进行读写操作
bool FileHandler::OpenFile(std::ios_base::openmode mode_) {
  this->open(this->fileheader.path, mode_);
  return is_open();
}

// 获取文件头信息
const FileHeader &FileHandler::getFileHeader() const { return fileheader; }

// 将文件头信息写入备份文件
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

// 打包普通文件
// 处理硬链接的特殊情况：对于同一个inode只保存一份数据
void RegularFileHandler::Pack(
    std::ofstream &backup_file,
    std::unordered_map<ino_t, std::string> &inode_table) {

  FileHeader header = this->getFileHeader();

  if (this->IsHardLink()) {
    // 如果是已存在的硬链接，只写入链接信息
    if (inode_table.count(header.metadata.st_ino)) {
      backup_file.write(reinterpret_cast<const char *>(&header),
                        sizeof(header));
      WriteLongPath(backup_file, inode_table[header.metadata.st_ino]);
      return;
    } else {
      // 第一次遇到该inode，记录路径
      header.metadata.st_nlink = 1;
      inode_table[header.metadata.st_ino] = std::string(header.path);
    }
  }

  // 写入文件头和内容
  backup_file.write(reinterpret_cast<const char *>(&header),
                    sizeof(header));

  if (!this->OpenFile()) {
    throw std::runtime_error("无法打开文件: " + std::string(header.path));
  }
  backup_file << this->rdbuf();
  this->close();
}

// 打包目录
// 只需保存目录的元数据信息
void DirectoryHandler::Pack(
    std::ofstream &backup_file,
    std::unordered_map<ino_t, std::string> &inode_table) {
  this->WriteHeader(backup_file);
}

// 打包符号链接
// 保存链接本身的元数据和目标路径
void SymlinkHandler::Pack(std::ofstream &backup_file,
                          std::unordered_map<ino_t, std::string> &inode_table) {
  this->WriteHeader(backup_file);
  FileHeader header = this->getFileHeader();
  std::string target_path = fs::read_symlink(header.path).string();
  WriteLongPath(backup_file, target_path);
}

// 解包普通文件
// 处理硬链接和普通文件的还原
void RegularFileHandler::Unpack(std::ifstream &backup_file, bool restore_metadata) {
  FileHeader header = this->getFileHeader();
  if (header.metadata.st_nlink > 1) {
    // 处理硬链接
    std::string target_path = ReadLongPath(backup_file);
    fs::path link_path = fs::current_path() / header.path;
    fs::path target = fs::current_path() / target_path;
    
    fs::create_directories(link_path.parent_path());
    if(fs::exists(link_path)) {
      fs::remove(link_path);
    }
    fs::create_hard_link(target, link_path);
    
    if (restore_metadata) {
      RestoreMetadata(link_path, header.metadata);
    }
    return;
  }

  // 处理普通文件
  fs::path output_path = fs::current_path() / header.path;
  fs::create_directories(output_path.parent_path());
  
  if(fs::exists(output_path)) {
    fs::remove(output_path);
  }
  
  std::ofstream output_file(output_path, std::ios::binary);
  if (!output_file) {
    throw std::runtime_error("无法创建文件: " + output_path.string());
  }

  // 按块读写文件内容
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

  if (restore_metadata) {
    RestoreMetadata(output_path, header.metadata);
  }
}

// 解包目录
// 创建目录并恢复其元数据
void DirectoryHandler::Unpack(std::ifstream &backup_file, bool restore_metadata) {
  FileHeader header = this->getFileHeader();
  fs::path dir_path = fs::current_path() / header.path;
  fs::create_directories(dir_path);
  
  if (restore_metadata) {
    RestoreMetadata(dir_path, header.metadata);
  }
}

// 解包符号链接
// 创建新的符号链接并恢复其元数据
void SymlinkHandler::Unpack(std::ifstream &backup_file, bool restore_metadata) {
  FileHeader header = this->getFileHeader();
  std::string target_path = ReadLongPath(backup_file);
  
  fs::path link_path = fs::current_path() / header.path;
  fs::create_directories(link_path.parent_path());

  if(fs::exists(link_path)) {
    fs::remove(link_path);
  }

  fs::create_symlink(target_path, link_path);
  
  if (restore_metadata) {
    RestoreMetadata(link_path, header.metadata);
  }
}

// 写入长路径到备份文件
void FileHandler::WriteLongPath(std::ofstream &backup_file, const std::string &path) const {
    // 先写入路径总长度
    uint32_t path_length = path.length();
    backup_file.write(reinterpret_cast<const char*>(&path_length), sizeof(path_length));
    
    // 写入实际路径
    backup_file.write(path.c_str(), path_length);
}

// 从备份文件读取长路径
std::string FileHandler::ReadLongPath(std::ifstream &backup_file) const {
    // 读取路径长度
    uint32_t path_length;
    backup_file.read(reinterpret_cast<char*>(&path_length), sizeof(path_length));
    
    // 读取实际路径
    std::vector<char> buffer(path_length + 1, '\0');
    backup_file.read(buffer.data(), path_length);
    
    return std::string(buffer.data(), path_length);
}

// 恢复文件的元数据
void FileHandler::RestoreMetadata(const fs::path& path, const struct stat& metadata) const {
  const char* path_str = path.c_str();

    // 还原文件权限信息
    if (chmod(path_str, metadata.st_mode & 07777) != 0) {  // 只还原权限位
        spdlog::warn("无法还原文件权限: {} ({})", path.string(), strerror(errno));
    }

    // 还原文件的用户和组
    if (lchown(path_str, metadata.st_uid, metadata.st_gid) != 0) {
        spdlog::warn("无法还原文件所有者: {} ({})", path.string(), strerror(errno));
    }

    // 还原时间戳(访问时间和修改时间)
    struct timespec times[2] = {metadata.st_atim, metadata.st_mtim};
    if (utimensat(AT_FDCWD, path_str, times, AT_SYMLINK_NOFOLLOW) != 0) {
        spdlog::warn("无法还原文件时间戳: {} ({})", path.string(), strerror(errno));
    }
}

// 打包管道文件
void FIFOHandler::Pack(std::ofstream &backup_file,
                      std::unordered_map<ino_t, std::string> &inode_table) {
    // 管道文件只需要保存文件头信息
    this->WriteHeader(backup_file);
}

// 解包管道文件
void FIFOHandler::Unpack(std::ifstream &backup_file, bool restore_metadata) {
    FileHeader header = this->getFileHeader();
    fs::path fifo_path = fs::current_path() / header.path;
    
    // 创建父目录
    fs::create_directories(fifo_path.parent_path());
    
    // 如果已存在则删除
    if(fs::exists(fifo_path)) {
        fs::remove(fifo_path);
    }
    
    // 创建管道文件
    if (mkfifo(fifo_path.c_str(), header.metadata.st_mode & 07777) != 0) {
        throw std::runtime_error("无法创建管道文件: " + fifo_path.string() + 
                               " (" + strerror(errno) + ")");
    }
    
    // 如果需要恢复元数据
    if (restore_metadata) {
        RestoreMetadata(fifo_path, header.metadata);
    }
}
