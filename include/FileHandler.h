#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unordered_map>

namespace fs = std::filesystem;

constexpr std::size_t MAX_PATH_LEN = 100;

/**
 * @brief 文件头部结构，存储文件路径和元数据
 */
struct FileHeader {
  char path[MAX_PATH_LEN];
  struct stat metadata;
};

/**
 * @brief 文件处理基类，提供文件操作的基本接口
 */
class FileHandler : public std::fstream {
public:
  // 默认构造函数
  FileHandler() = default;
  FileHandler(const FileHeader &header) : fileheader(header) {}

  // 根据路径构造
  explicit FileHandler(const fs::path &filepath);

  // 根据FileHeader构造

  /**
   * @brief 创建适当类型的文件处理器
   * @param path 文件路径
   * @return 文件处理器的智能指针
   */
  static std::unique_ptr<FileHandler> Create(const fs::path& path);
  static std::unique_ptr<FileHandler> Create(const FileHeader &header);

  /**
   * @brief 打包文件
   * @param backup_file 备份文件流
   * @param inode_table inode表，用于处理硬链接
   */
  virtual void Pack(std::ofstream &backup_file,
                    std::unordered_map<ino_t, std::string> &inode_table) = 0;
  /**
   * @brief 解包文件
   * @param backup_file 备份文件流
   * @param restore_metadata 是否恢复元数据
   */
  virtual void Unpack(std::ifstream &backup_file, bool restore_metadata = false) = 0;
  virtual ~FileHandler() = default;

private:
  FileHeader fileheader;

protected:
  bool IsHardLink() const;
  bool OpenFile(std::ios_base::openmode mode_ = std::ios_base::in |
                                                std::ios_base::binary);
  const FileHeader &getFileHeader() const;
  void WriteHeader(std::ofstream &backup_file) const;

  // 添加一个辅助函数来处理长路径
  void WriteLongPath(std::ofstream &backup_file, const std::string &path) const;
  // 读取可能超过MAX_PATH_LEN的路径
  std::string ReadLongPath(std::ifstream &backup_file) const;

  void RestoreMetadata(const fs::path& path, const struct stat& metadata) const;
};

class RegularFileHandler : public FileHandler {
public:
  RegularFileHandler() : FileHandler() {}
  RegularFileHandler(const fs::path &path) : FileHandler(path) {}
  RegularFileHandler(const FileHeader &header) : FileHandler(header) {}

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(std::ifstream &backup_file, bool restore_metadata = false) override;
};

class DirectoryHandler : public FileHandler {
public:
  DirectoryHandler() : FileHandler() {}
  DirectoryHandler(const fs::path &path) : FileHandler(path) {}
  DirectoryHandler(const FileHeader &header) : FileHandler(header) {}

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(std::ifstream &backup_file, bool restore_metadata = false) override;
};

class SymlinkHandler : public FileHandler {
public:
  SymlinkHandler() : FileHandler() {}
  SymlinkHandler(const fs::path &path) : FileHandler(path) {}
  SymlinkHandler(const FileHeader &header) : FileHandler(header) {}

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(std::ifstream &backup_file, bool restore_metadata = false) override;
};

class FIFOHandler : public FileHandler {
public:
    FIFOHandler() : FileHandler() {}
    FIFOHandler(const fs::path &path) : FileHandler(path) {}
    FIFOHandler(const FileHeader &header) : FileHandler(header) {}

    void Pack(std::ofstream &backup_file,
              std::unordered_map<ino_t, std::string> &inode_table) override;
    void Unpack(std::ifstream &backup_file, bool restore_metadata = false) override;
};

#endif // FILE_HANDLER_H
