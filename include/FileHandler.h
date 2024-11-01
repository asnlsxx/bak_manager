#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unordered_map>

// #include "FileBase.h"

namespace fs = std::filesystem;

constexpr std::size_t MAX_PATH_LEN = 100;

struct FileHeader {
  char path[MAX_PATH_LEN];
  struct stat metadata;
};

class FileHandler : public std::fstream {
public:
  // 默认构造函数
  FileHandler() = default;

  // 根据路径构造
  explicit FileHandler(const fs::path &filepath);

  static std::unique_ptr<FileHandler> Create(const fs::path& path);
  static std::unique_ptr<FileHandler> Create(const FileHeader &header);

  // TODO 是否可以将 table 放在 Pack 的全局空间中？
  virtual void Pack(std::ofstream &backup_file,
                    std::unordered_map<ino_t, std::string> &inode_table) = 0;
  virtual void Unpack(const FileHeader &header, std::ifstream &backup_file) = 0;
  virtual ~FileHandler() = default;

private:
  FileHeader fileheader;

protected:
  bool IsHardLink() const;
  bool OpenFile(std::ios_base::openmode mode_ = std::ios_base::in |
                                                std::ios_base::binary);
  const FileHeader &getFileHeader() const;
  void WriteHeader(std::ofstream &backup_file) const;
};

class RegularFileHandler : public FileHandler {
public:
  RegularFileHandler() : FileHandler() {} // 显式调用父类构造函数
  RegularFileHandler(const fs::path &path)
      : FileHandler(path) {} // 显式调用父类带参构造函数

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

class DirectoryHandler : public FileHandler {
public:
  DirectoryHandler() : FileHandler() {} // 显式调用父类构造函数
  DirectoryHandler(const fs::path &path)
      : FileHandler(path) {} // 显式调用父类带参构造函数

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

class SymlinkHandler : public FileHandler {
public:
  SymlinkHandler() : FileHandler() {} // 显式调用父类构造函数
  SymlinkHandler(const fs::path &path)
      : FileHandler(path) {} // 显式调用父类带参构造函数

  void Pack(std::ofstream &backup_file,
            std::unordered_map<ino_t, std::string> &inode_table) override;
  void Unpack(const FileHeader &header, std::ifstream &backup_file) override;
};

#endif // FILE_HANDLER_H
